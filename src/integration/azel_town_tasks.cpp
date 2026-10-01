#include "lagi/azel_town_tasks.h"

#include "lagi/azel_compat.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/azel_town_runtime.h"
#include "lagi/azel_town_collision.h"
#include "lagi/azel_render_bridge.h"
#include "lagi/debug_mesh.h"
#include "lagi/platform.h"
#include "task.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

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

struct TownWorldCellTask;
struct WorldGridState {
    bool initialized = false;
    std::int8_t bundleIndex = -1;
    int ringX = 0;
    int ringY = 0;
    int currentX = 0;
    int currentY = 0;
    TownWorldCellTask* cells[8][8]{};
    std::array<std::vector<u32>, 64> objectLists{};
};

WorldGridState g_worldGrid{};
StaticRoomDebugMesh g_worldScene{};
p_workArea g_worldParent = nullptr;

struct EdgeRuntimeState {
    bool initialized = false;
    float position[3]{};
    float startPosition[3]{};
    float yaw = 0.0f;
    float startYaw = 0.0f;
    TownCollisionBody collision{};
};

EdgeRuntimeState g_edge{};
bool g_pendingEdgePosition = false;
bool g_pendingEdgeOrientation = false;
float g_pendingEdgePositionValue[3]{};
float g_pendingEdgeYaw = 0.0f;

static s32 initNpcWorld(s32 setupIndex);
static s32 initNpcFromStructWorld(u32 objectEA);
static s32 updateWorldGridRaw(s32 x, s32 z);

static float wrapAngle(float value)
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTau = kPi * 2.0f;
    while (value > kPi) value -= kTau;
    while (value < -kPi) value += kTau;
    return value;
}

static void presentEdge()
{
    platform::renderer::town_present_edge(
        g_edge.position[0], g_edge.position[1], g_edge.position[2],
        g_edge.yaw,
        (g_edge.collision.contactMask & 0x4u) != 0,
        g_edge.collision.contactCount);
}

static void setEdgePositionRaw(s32 x, s32 y, s32 z)
{
    constexpr float kFixed = 1.0f / 65536.0f;
    g_pendingEdgePosition = true;
    g_pendingEdgePositionValue[0] = static_cast<float>(x) * kFixed;
    g_pendingEdgePositionValue[1] = static_cast<float>(y) * kFixed;
    g_pendingEdgePositionValue[2] = static_cast<float>(z) * kFixed;
    std::copy(std::begin(g_pendingEdgePositionValue),
              std::end(g_pendingEdgePositionValue),
              std::begin(g_edge.position));
    presentEdge();
}

static void setEdgeOrientationRaw(s32, s32 y, s32)
{
    constexpr float kTau = 6.28318530717958647692f;
    g_pendingEdgeOrientation = true;
    g_pendingEdgeYaw = static_cast<float>(y) /
        static_cast<float>(0x10000000) * kTau;
    g_edge.yaw = g_pendingEdgeYaw;
    presentEdge();
}

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
    case 0x0600CCB4u:
        return argc == 1 ? initNpcWorld(args[0]) : 0;
    case 0x06014DF2u:
        return argc == 1 ? initNpcFromStructWorld(static_cast<u32>(args[0])) : 0;
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
            setEdgePositionRaw(args[1], args[2], args[3]);
        return 0;
    case 0x0605AF0Eu:
        if (argc == 4 && args[0] == 0)
            setEdgeOrientationRaw(args[1], args[2], args[3]);
        return 0;
    case 0x060144C0u:
        return argc == 2 ? updateWorldGridRaw(args[0], args[1]) : 0;
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
    static void Init(TownScriptTask* self) {
        resetCollisionFrame();
        g_worldParent = self;
    }
    static void UpdateTask(TownScriptTask*) {
        if (platform::renderer::town_scene_active()) {
            processAllCollisions();
            resetCollisionFrame();
        }
        runScript();
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, UpdateTask, nullptr, nullptr};
        return &definition;
    }
};

struct TownWorldCellTask final : s_workAreaTemplateWithArg<TownWorldCellTask, int> {
    int cellIndex = -1;

    static void Init(TownWorldCellTask* self, int index) {
        self->cellIndex = index;
    }
    static void DrawTask(TownWorldCellTask* self) {
        if (self->cellIndex != town_runtime().activeCellIndex)
            return;
        for (std::size_t i = 0; i < g_worldScene.objectStates.size(); ++i) {
            const auto& object = g_worldScene.objectStates[i];
            azel_bridge::SubmissionState state{};
            state.modelMatrix[0] = state.modelMatrix[5] =
                state.modelMatrix[10] = 0x10000;
            state.hasModelMatrix = true;
            if (g_worldScene.lightingValid) {
                state.lightVector[0] = static_cast<s32>(
                    -g_worldScene.lightDirection[0] * 65536.0f);
                state.lightVector[1] = static_cast<s32>(
                    -g_worldScene.lightDirection[1] * 65536.0f);
                state.lightVector[2] = static_cast<s32>(
                    -g_worldScene.lightDirection[2] * 65536.0f);
                state.lightColor[0] = g_worldScene.lightColor[0];
                state.lightColor[1] = g_worldScene.lightColor[1];
                state.lightColor[2] = g_worldScene.lightColor[2];
                state.hasLight = true;
            }
            azel_bridge::submit_town_object(
                static_cast<u32>(self->cellIndex),
                static_cast<u32>(i),
                object.firstPolygon,
                object.polygonCount,
                state);
        }
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, nullptr, DrawTask, nullptr};
        return &definition;
    }
};

static void deleteCell(int x, int y)
{
    if (x < 0 || y < 0 || x >= town_runtime().gridWidth ||
        y >= town_runtime().gridHeight)
        return;
    TownWorldCellTask*& task =
        g_worldGrid.cells[(g_worldGrid.ringY + y - g_worldGrid.currentY) & 7]
                         [(g_worldGrid.ringX + x - g_worldGrid.currentX) & 7];
    if (task)
        task->getTask()->markFinished();
    task = nullptr;
}

static void createCell(int x, int y)
{
    const auto& runtime = town_runtime();
    if (!g_worldParent || x < 0 || y < 0 ||
        x >= runtime.gridWidth || y >= runtime.gridHeight)
        return;
    const int index = y * runtime.gridWidth + x;
    if (index < 0 || index >= static_cast<int>(runtime.cells.size()) ||
        !runtime.cells[static_cast<std::size_t>(index)].valid)
        return;
    TownWorldCellTask*& slot =
        g_worldGrid.cells[(g_worldGrid.ringY + y - g_worldGrid.currentY) & 7]
                         [(g_worldGrid.ringX + x - g_worldGrid.currentX) & 7];
    if (!slot)
        slot = createSubTaskWithArg<TownWorldCellTask>(g_worldParent, index);
}

static void deleteAllGridCells()
{
    for (auto& row : g_worldGrid.cells)
        for (auto*& cell : row) {
            if (cell)
                cell->getTask()->markFinished();
            cell = nullptr;
        }
}

static void createGridCells(int centerX, int centerY)
{
    g_worldGrid.ringX = 0;
    g_worldGrid.ringY = 0;
    g_worldGrid.currentX = centerX;
    g_worldGrid.currentY = centerY;
    for (int y = -2; y <= 2; ++y)
        for (int x = -2; x <= 2; ++x)
            createCell(centerX + x, centerY + y);
}

static void resetWorldGrid()
{
    deleteAllGridCells();
    if (g_worldGrid.bundleIndex >= 0)
        release_town_runtime_bundle(g_worldGrid.bundleIndex);
    g_worldGrid = {};
    g_worldGrid.bundleIndex = -1;
    g_worldScene = {};
}

static s32 initNpcWorld(s32 setupIndex)
{
    const auto& runtime = town_runtime();
    if (setupIndex != 0 || runtime.setupNpcFileIndex < 0)
        return 0;

    resetWorldGrid();
    if (!acquire_town_runtime_bundle(runtime.setupNpcFileIndex))
        return 0;
    g_worldGrid.bundleIndex = runtime.setupNpcFileIndex;

    if (!build_town_world_scene(g_worldScene) ||
        !platform::renderer::load_static_room_viewer(g_worldScene)) {
        resetWorldGrid();
        return 0;
    }

    g_edge = {};
    g_edge.initialized = g_worldScene.edgeTransformValid;
    if (g_edge.initialized) {
        std::copy(std::begin(g_worldScene.edgePosition),
                  std::end(g_worldScene.edgePosition),
                  std::begin(g_edge.position));
        std::copy(std::begin(g_worldScene.edgePosition),
                  std::end(g_worldScene.edgePosition),
                  std::begin(g_edge.startPosition));
        constexpr float kTau = 6.28318530717958647692f;
        g_edge.yaw = g_worldScene.edgeRotation[1] * kTau;
        if (g_pendingEdgePosition)
            std::copy(std::begin(g_pendingEdgePositionValue),
                      std::end(g_pendingEdgePositionValue),
                      std::begin(g_edge.position));
        if (g_pendingEdgeOrientation)
            g_edge.yaw = g_pendingEdgeYaw;
        std::copy(std::begin(g_edge.position),
                  std::end(g_edge.position),
                  std::begin(g_edge.startPosition));
        g_edge.startYaw = g_edge.yaw;
        if (g_worldScene.edgeCollisionValid) {
            setCollisionSetup(g_edge.collision, 0);
            setCollisionBounds(
                g_edge.collision,
                {g_worldScene.edgeCollisionMin[0],
                 g_worldScene.edgeCollisionMin[1],
                 g_worldScene.edgeCollisionMin[2]},
                {g_worldScene.edgeCollisionMax[0],
                 g_worldScene.edgeCollisionMax[1],
                 g_worldScene.edgeCollisionMax[2]});
        }
        presentEdge();
    }

    g_worldGrid.initialized = true;
    createGridCells(-3, -3);
    platform::logging::writef(
        "[TownWorld] initNPC setup=%d bundle=%d grid=%dx%d refs=%u\n",
        static_cast<int>(setupIndex),
        static_cast<int>(runtime.setupNpcFileIndex),
        static_cast<int>(runtime.gridWidth),
        static_cast<int>(runtime.gridHeight),
        town_runtime_bundle(runtime.setupNpcFileIndex)->refCount);
    return 0;
}

static s32 initNpcFromStructWorld(u32 objectEA)
{
    if (!g_worldGrid.initialized || !objectEA)
        return 0;
    sSaturnMemoryFile* overlay = town_overlay_file();
    if (!overlay)
        return 0;
    const sSaturnPtr object = overlay->getSaturnPtr(objectEA);
    const float x = static_cast<float>(readSaturnS32(object + 8)) / 65536.0f;
    const float z = static_cast<float>(readSaturnS32(object + 0x10)) / 65536.0f;
    const auto& runtime = town_runtime();
    const int cellX = std::clamp(
        static_cast<int>(x / runtime.gridCellSize),
        0, static_cast<int>(runtime.gridWidth) - 1);
    const int cellY = std::clamp(
        static_cast<int>(z / runtime.gridCellSize),
        0, static_cast<int>(runtime.gridHeight) - 1);
    g_worldGrid.objectLists[static_cast<std::size_t>(
        cellY * runtime.gridWidth + cellX)].push_back(objectEA);
    return 1;
}

static s32 updateWorldGridRaw(s32 x, s32 z)
{
    if (!g_worldGrid.initialized || town_runtime().gridCellSize <= 0.0f)
        return 0;
    const float worldX = static_cast<float>(x) / 65536.0f;
    const float worldZ = static_cast<float>(z) / 65536.0f;
    const int nextX = static_cast<int>(worldX / town_runtime().gridCellSize);
    const int nextY = static_cast<int>(worldZ / town_runtime().gridCellSize);
    const int dx = nextX - g_worldGrid.currentX;
    const int dy = nextY - g_worldGrid.currentY;

    if (std::abs(dx) > 1 || std::abs(dy) > 1) {
        deleteAllGridCells();
        createGridCells(nextX, nextY);
    } else {
        if (dx < 0) {
            for (int y = -2; y <= 2; ++y)
                deleteCell(g_worldGrid.currentX + 2, g_worldGrid.currentY + y);
            g_worldGrid.ringX = (g_worldGrid.ringX - 1) & 7;
            --g_worldGrid.currentX;
            for (int y = -2; y <= 2; ++y)
                createCell(g_worldGrid.currentX - 2, g_worldGrid.currentY + y);
        } else if (dx > 0) {
            for (int y = -2; y <= 2; ++y)
                deleteCell(g_worldGrid.currentX - 2, g_worldGrid.currentY + y);
            g_worldGrid.ringX = (g_worldGrid.ringX + 1) & 7;
            ++g_worldGrid.currentX;
            for (int y = -2; y <= 2; ++y)
                createCell(g_worldGrid.currentX + 2, g_worldGrid.currentY + y);
        }
        if (dy < 0) {
            for (int cellX = -2; cellX <= 2; ++cellX)
                deleteCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY + 2);
            g_worldGrid.ringY = (g_worldGrid.ringY - 1) & 7;
            --g_worldGrid.currentY;
            for (int cellX = -2; cellX <= 2; ++cellX)
                createCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY - 2);
        } else if (dy > 0) {
            for (int cellX = -2; cellX <= 2; ++cellX)
                deleteCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY - 2);
            g_worldGrid.ringY = (g_worldGrid.ringY + 1) & 7;
            ++g_worldGrid.currentY;
            for (int cellX = -2; cellX <= 2; ++cellX)
                createCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY + 2);
        }
    }
    update_town_runtime_active_cell(worldX, worldZ);
    return 0;
}

struct RuinBackgroundTask final : s_workAreaTemplate<RuinBackgroundTask> {
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<RuinBackgroundTask>(nullptr); }
};

struct TownEdgeTask final : s_workAreaTemplate<TownEdgeTask> {
    static void UpdateTask(TownEdgeTask*) {
        if (!g_edge.initialized || !platform::renderer::town_scene_active())
            return;

        if (platform::input::reset_view_pressed()) {
            std::copy(std::begin(g_edge.startPosition),
                      std::end(g_edge.startPosition),
                      std::begin(g_edge.position));
            g_edge.yaw = g_edge.startYaw;
        }

        float forwardX = 0.0f, forwardZ = 1.0f;
        platform::renderer::town_camera_forward(forwardX, forwardZ);
        float forwardLength = std::sqrt(
            forwardX * forwardX + forwardZ * forwardZ);
        if (forwardLength <= 0.0001f) {
            forwardX = std::sin(g_edge.yaw);
            forwardZ = std::cos(g_edge.yaw);
            forwardLength = 1.0f;
        }
        forwardX /= forwardLength;
        forwardZ /= forwardLength;

        const float inputX = platform::input::analog_x();
        const float inputForward = -platform::input::analog_y();
        const float magnitude = std::min(
            1.0f, std::sqrt(inputX * inputX + inputForward * inputForward));
        if (magnitude > 0.0001f) {
            const float rightX = forwardZ;
            const float rightZ = -forwardX;
            float desiredX = rightX * -inputX + forwardX * inputForward;
            float desiredZ = rightZ * inputX + forwardZ * inputForward;
            const float desiredLength = std::sqrt(
                desiredX * desiredX + desiredZ * desiredZ);
            if (desiredLength > 0.0001f) {
                desiredX /= desiredLength;
                desiredZ /= desiredLength;
                const float desiredYaw = std::atan2(-desiredX, -desiredZ);
                constexpr float kTau = 6.28318530717958647692f;
                constexpr float kMaxTurn =
                    static_cast<float>(0x0E38E3) /
                    static_cast<float>(0x10000000) * kTau;
                const float delta = std::clamp(
                    wrapAngle(desiredYaw - g_edge.yaw),
                    -kMaxTurn, kMaxTurn);
                g_edge.yaw = wrapAngle(g_edge.yaw + delta);

                constexpr float kWalkStep =
                    static_cast<float>(0x109) / 65536.0f;
                const float step = kWalkStep * magnitude;
                g_edge.position[0] -= std::sin(g_edge.yaw) * step;
                g_edge.position[2] -= std::cos(g_edge.yaw) * step;
            }
        }

        const auto& solve = g_edge.collision.collisionSolveTranslation;
        g_edge.position[0] += solve.x;
        g_edge.position[1] += solve.y;
        g_edge.position[2] += solve.z;
        g_edge.collision.ownerPosition = {
            g_edge.position[0], g_edge.position[1], g_edge.position[2]};
        g_edge.collision.ownerRotation = {0.0f, g_edge.yaw, 0.0f};
        registerCollisionBody(g_edge.collision);
        presentEdge();
    }
    static void DrawTask(TownEdgeTask*) { platform::renderer::town_camera_update(); }
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<TownEdgeTask>(UpdateTask, DrawTask); }
};

struct TownMainLogicTask final : s_workAreaTemplate<TownMainLogicTask> {
    static void UpdateTask(TownMainLogicTask*) {
        const int x = static_cast<int>(
            std::lround(g_edge.position[0] * 65536.0f));
        const int z = static_cast<int>(
            std::lround(g_edge.position[2] * 65536.0f));
        updateWorldGridRaw(x, z);
        platform::renderer::town_main_logic_update();
    }
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
        g_edge = {};
        g_pendingEdgePosition = false;
        g_pendingEdgeOrientation = false;
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
