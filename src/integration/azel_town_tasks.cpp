#include "lagi/azel_town_tasks.h"

#include "lagi/azel_compat.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/azel_town_runtime.h"
#include "lagi/azel_town_collision.h"
#include "lagi/platform.h"
#include "task.h"

#include <array>
#include <cstdio>

s8 readSaturnS8(sSaturnPtr ptr);
u8 readSaturnU8(sSaturnPtr ptr);
s16 readSaturnS16(sSaturnPtr ptr);
u16 readSaturnU16(sSaturnPtr ptr);
s32 readSaturnS32(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);

namespace lagi::azel {
namespace {

struct ScriptContext {
    sSaturnPtr pc{};
    std::array<sSaturnPtr, 16> stack{};
    unsigned stackPointer = 16;
    s32 result = 0;
    u16 waitFrames = 0;
    bool halted = false;
};

std::array<u8, 0x300> g_gameBits{};
ScriptContext g_script{};
unsigned g_pipelineFrames = 0;

static sSaturnPtr align2(sSaturnPtr ptr)
{
    ptr.m_offset = (ptr.m_offset + 1) & ~1;
    return ptr;
}

static sSaturnPtr align4(sSaturnPtr ptr)
{
    ptr.m_offset = (ptr.m_offset + 3) & ~3;
    return ptr;
}

static bool pointerValid(sSaturnPtr ptr, u32 bytes = 1)
{
    if (!ptr.m_file || !ptr.m_file->m_data || ptr.m_offset < ptr.m_file->m_base)
        return false;
    const u32 offset = static_cast<u32>(ptr.m_offset) - ptr.m_file->m_base;
    return offset <= ptr.m_file->m_dataSize &&
           bytes <= ptr.m_file->m_dataSize - offset;
}

static unsigned adjustedBit(s16 encoded)
{
    return static_cast<unsigned>(encoded < 1000 ? encoded + 3334 : encoded);
}

static bool getBit(unsigned bit)
{
    return bit < g_gameBits.size() * 8u &&
           (g_gameBits[bit / 8u] & (0x80u >> (bit & 7u))) != 0;
}

static void setBit(unsigned bit, bool value)
{
    if (bit >= g_gameBits.size() * 8u)
        return;
    const u8 mask = static_cast<u8>(0x80u >> (bit & 7u));
    if (value) g_gameBits[bit / 8u] |= mask;
    else g_gameBits[bit / 8u] &= static_cast<u8>(~mask);
}

static u32 readPackedBits(unsigned first, unsigned count)
{
    u32 value = 0;
    for (unsigned i = 0; i < count && i < 32; ++i)
        value = (value << 1) | (getBit(first + i) ? 1u : 0u);
    return value;
}

static void writePackedBits(unsigned first, unsigned count, u32 value)
{
    for (unsigned i = 0; i < count && i < 32; ++i)
        setBit(first + i, (value & (1u << (count - 1u - i))) != 0);
}

static s32 dispatchNative(u32 functionEA, unsigned argc, const s32* args)
{
    switch (functionEA) {
    case 0x0600CCB4u: // initNPC -- TownRuntime already owns this setup.
    case 0x06014DF2u: // initNPCFromStruct
    case 0x06027110u: // setNextGameStatus
    case 0x0602C1D0u: // fadeOutAllSequences
    case 0x0602C2CAu: // playSystemSoundEffect
    case 0x0602C32Au: // playBattleSoundEffect
    case 0x0603011Eu: // terminateTown
    case 0x0605C83Cu: // TwnFadeOut
    case 0x0605C7C4u: // TwnFadeIn
    case 0x06054364u: // waitForLockDisableCompletion
    case 0x06054334u: // disableLock
    case 0x0605B320u: // NPC control
    case 0x0605C55Cu: // townCamera_setup; decoded scene lighting owns it today.
        return 0;
    case 0x0600CDD4u: // getNpcData0_5d
        return 0;
    case 0x0605AEE0u:
        if (argc == 4 && args[0] == 0)
            platform::renderer::town_edge_set_position(args[1], args[2], args[3]);
        return 0;
    case 0x0605AF0Eu:
        if (argc == 4 && args[0] == 0)
            platform::renderer::town_edge_set_orientation(args[1], args[2], args[3]);
        return 0;
    case 0x060144C0u:
        if (argc == 2)
            update_town_runtime_active_cell(
                static_cast<float>(args[0]) / 65536.0f,
                static_cast<float>(args[1]) / 65536.0f);
        return 0;
    case 0x0600CC78u: // setSomethingInNpc0
    case 0x06057570u: // hasLoadingCompleted
    case 0x06057058u: // setupCameraFollowMode / NPC walk setup
    case 0x0605762Au: // setupAutoWalk
        return functionEA == 0x06057570u ? 1 : 0;
    case 0x0605800Eu: // isObjectCloseEnoughToActivate
        return 1;
    default:
        platform::logging::writef(
            "[TownScript] unported native %08X argc=%u at %08X; script paused\n",
            static_cast<unsigned>(functionEA), argc,
            static_cast<unsigned>(g_script.pc.m_offset));
        g_script.halted = true;
        return 0;
    }
}

static sSaturnPtr callNative(sSaturnPtr ptr)
{
    const unsigned argc = readSaturnU8(ptr);
    sSaturnPtr data = align4(ptr + 1);
    const u32 functionEA = static_cast<u32>(readSaturnEA(data).m_offset);
    s32 args[4]{};
    if (argc > 4) {
        g_script.halted = true;
        return {};
    }
    for (unsigned i = 0; i < argc; ++i)
        args[i] = readSaturnS32(data + static_cast<int>(4 + i * 4));
    g_script.result = dispatchNative(functionEA, argc, args);
    return data + static_cast<int>((argc + 1) * 4);
}

static void runScript()
{
    if (g_script.halted || g_script.pc.isNull())
        return;
    if (g_script.waitFrames) {
        --g_script.waitFrames;
        return;
    }

    sSaturnPtr pc = g_script.pc;
    for (unsigned operations = 0; operations < 512; ++operations) {
        if (!pointerValid(pc)) {
            g_script.halted = true;
            break;
        }
        const u8 opcode = readSaturnU8(pc);
        pc += 1u;
        switch (opcode) {
        case 1:
            if (g_script.stackPointer == g_script.stack.size()) {
                g_script.pc = {};
                return;
            }
            pc = g_script.stack[g_script.stackPointer++];
            break;
        case 2:
            pc = align2(pc);
            g_script.waitFrames = static_cast<u16>(readSaturnU16(pc) - 1u);
            g_script.pc = pc + 2;
            return;
        case 3: pc = readSaturnEA(align4(pc)); break;
        case 5:
            pc = g_script.result ? align4(pc + 4) : readSaturnEA(align4(pc));
            break;
        case 6: {
            sSaturnPtr address = align4(pc);
            if (g_script.stackPointer == 0) { g_script.halted = true; break; }
            g_script.stack[--g_script.stackPointer] = address + 4;
            pc = readSaturnEA(address);
            break;
        }
        case 7: pc = callNative(pc); break;
        case 8: pc = align2(pc); g_script.result = readSaturnS16(pc) == g_script.result; pc += 2u; break;
        case 9: pc = align2(pc); g_script.result = readSaturnS16(pc) != g_script.result; pc += 2u; break;
        case 10: pc = align2(pc); g_script.result = readSaturnS16(pc) < g_script.result; pc += 2u; break;
        case 11: pc = align2(pc); g_script.result = readSaturnS16(pc) <= g_script.result; pc += 2u; break;
        case 12: pc = align2(pc); g_script.result = readSaturnS16(pc) > g_script.result; pc += 2u; break;
        case 13: pc = align2(pc); g_script.result = readSaturnS16(pc) >= g_script.result; pc += 2u; break;
        case 14: pc = align2(pc); g_script.result += readSaturnS16(pc); pc += 2u; break;
        case 15: pc = align2(pc); setBit(adjustedBit(readSaturnS16(pc)), true); pc += 2u; break;
        case 16: pc = align2(pc); setBit(adjustedBit(readSaturnS16(pc)), false); pc += 2u; break;
        case 17: pc = align2(pc); g_script.result = getBit(adjustedBit(readSaturnS16(pc))); pc += 2u; break;
        case 18: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align2(pc);
            const unsigned first = adjustedBit(readSaturnS16(pc)); pc += 2u;
            g_script.result = static_cast<s32>(readPackedBits(first, count));
            break;
        }
        case 20: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align2(pc);
            const unsigned first = adjustedBit(readSaturnS16(pc)); pc += 2u;
            const s16 delta = readSaturnS16(pc); pc += 2u;
            writePackedBits(first, count, readPackedBits(first, count) + delta);
            break;
        }
        case 21: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align4(pc);
            pc = g_script.result >= 0 && static_cast<unsigned>(g_script.result) < readSaturnU8(pc)
                ? readSaturnEA(pc + g_script.result * 4)
                : pc + static_cast<int>(count * 4);
            break;
        }
        case 22: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align4(pc);
            if (g_script.result >= 0 && static_cast<unsigned>(g_script.result) < count) {
                if (g_script.stackPointer == 0) { g_script.halted = true; break; }
                g_script.stack[--g_script.stackPointer] = pc + static_cast<int>(count * 4);
                pc = readSaturnEA(pc + g_script.result * 4);
            } else pc += static_cast<unsigned>(count * 4);
            break;
        }
        case 24: g_script.result = 0; break;
        case 25: case 26: case 27: case 29: case 32: case 33: case 34:
            if (opcode == 27) pc = align4(pc) + 4;
            else if (opcode == 33 || opcode == 34) pc += 1u;
            break;
        case 31: {
            const sSaturnPtr retry = pc - 1u;
            pc = callNative(pc);
            if (!g_script.result) { g_script.pc = retry; return; }
            break;
        }
        case 36: pc = align2(pc) + 2; pc = align4(pc) + 4; break;
        case 46: pc = align2(pc) + 2; break;
        default:
            platform::logging::writef(
                "[TownScript] unsupported opcode %u at %08X; script paused\n",
                static_cast<unsigned>(opcode),
                static_cast<unsigned>(pc.m_offset - 1));
            g_script.halted = true;
            break;
        }
        if (g_script.halted)
            break;
    }
    g_script.pc = pc;
}

template<class T> static const typename T::TypedTaskDefinition* taskDefinition(
    typename T::FunctionType update,
    typename T::FunctionType draw = nullptr)
{
    static typename T::TypedTaskDefinition definition{nullptr, update, draw, nullptr};
    return &definition;
}

struct TownScriptTask final : s_workAreaTemplate<TownScriptTask> {
    static void Init(TownScriptTask*) { resetCollisionFrame(); }
    static void UpdateTask(TownScriptTask*) {
        platform::renderer::town_script_collision_update();
        runScript();
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, UpdateTask, nullptr, nullptr};
        return &definition;
    }
};

struct RuinBackgroundTask final : s_workAreaTemplate<RuinBackgroundTask> {
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<RuinBackgroundTask>(nullptr); }
};

struct TownEdgeTask final : s_workAreaTemplate<TownEdgeTask> {
    static void UpdateTask(TownEdgeTask*) { platform::renderer::town_edge_update(); }
    static void DrawTask(TownEdgeTask*) { platform::renderer::town_camera_update(); }
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<TownEdgeTask>(UpdateTask, DrawTask); }
};

struct TownMainLogicTask final : s_workAreaTemplate<TownMainLogicTask> {
    static void UpdateTask(TownMainLogicTask*) { platform::renderer::town_main_logic_update(); }
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<TownMainLogicTask>(UpdateTask); }
};

struct TownCameraTask final : s_workAreaTemplate<TownCameraTask> {
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<TownCameraTask>(nullptr); }
};

struct TwnRuinTask final : s_workAreaTemplate<TwnRuinTask> {
    static void Init(TwnRuinTask* self) {
        sSaturnMemoryFile* overlay = town_overlay_file();
        const TownRuntimeState& town = town_runtime();
        g_script = {};
        g_gameBits.fill(0);
        g_pipelineFrames = 0;
        if (overlay && town.initialScriptEA)
            g_script.pc = overlay->getSaturnPtr(town.initialScriptEA);

        // overlayStart_TWN_RUIN creates these siblings in this exact order.
        createSubTask<TownScriptTask>(self);
        createSubTask<RuinBackgroundTask>(self);
        createSubTask<TownEdgeTask>(self);
        createSubTask<TownMainLogicTask>(self);
        createSubTask<TownCameraTask>(self);
        platform::logging::writef(
            "[TownTasks] overlayStart_TWN_RUIN script=%08X children=script/background/edge/main/camera\n",
            static_cast<unsigned>(town.initialScriptEA));
    }
    static void UpdateTask(TwnRuinTask*) { ++g_pipelineFrames; }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, UpdateTask, nullptr, nullptr};
        return &definition;
    }
};

TwnRuinTask* g_twnRuinTask = nullptr;

} // namespace

bool start_twn_ruin_task_pipeline()
{
    if (!town_runtime().initialized || !town_overlay_file())
        return false;
    g_twnRuinTask = createRootTask<TwnRuinTask>();
    return g_twnRuinTask != nullptr;
}

bool twn_ruin_task_pipeline_alive()
{
    return g_twnRuinTask && g_pipelineFrames > 0;
}

} // namespace lagi::azel
