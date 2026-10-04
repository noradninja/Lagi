#include <cstdio>
#include <psp2/kernel/processmgr.h>

#include "lagi/lagi_compat.h"
#include "lagi/platform.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/disc_image.h"
#include "lagi/lagi_azel_upstream_prelude.h"

#include "task.h"
#include "rootTask.h"
#include "VDP1.h"
#include "VDP2.h"
#include "movie/movie.h"
#include "titleScreen.h"
#include "kernel/moduleManager.h"
#include "battle/BTL_A3/BTL_A3_map6.h"

extern int numActiveTask;
void azelInit();
void resetEngine();
void updateFadeInterrupt();
void updateInputs();
p_workArea createTitleMenuTask(p_workArea);

namespace lagi::azel {

static bool saturn_memory_smoke_test()
{
    constexpr u32 base = 0x06054000;
    u8 data[40] = {};

    data[0] = 0x12; data[1] = 0x34;
    data[4] = 0x00; data[5] = 0x01; data[6] = 0x00; data[7] = 0x00;
    data[8] = 'A'; data[9] = 'Z'; data[10] = 'E'; data[11] = 'L'; data[12] = 0;

    const u32 target = base + 24;
    data[16] = static_cast<u8>(target >> 24);
    data[17] = static_cast<u8>(target >> 16);
    data[18] = static_cast<u8>(target >> 8);
    data[19] = static_cast<u8>(target);

    const s32 vec[3] = { 0x00010000, -0x00020000, 0x00008000 };
    for (int i = 0; i < 3; ++i) {
        const u32 v = static_cast<u32>(vec[i]);
        data[24 + i * 4 + 0] = static_cast<u8>(v >> 24);
        data[24 + i * 4 + 1] = static_cast<u8>(v >> 16);
        data[24 + i * 4 + 2] = static_cast<u8>(v >> 8);
        data[24 + i * 4 + 3] = static_cast<u8>(v);
    }

    sSaturnMemoryFile file;
    file.m_name = "Lagi synthetic Saturn block";
    file.m_data = data;
    file.m_dataSize = sizeof(data);
    file.m_base = base;

    const sSaturnPtr root = file.getSaturnPtr(base);
    if (readSaturnU16(root) != 0x1234) return false;
    if (readSaturnFP(root + 4).asS32() != 0x00010000) return false;
    if (readSaturnString(root + 8) != "AZEL") return false;

    const sSaturnPtr ea = readSaturnEA(root + 16);
    const sVec3_FP v = readSaturnVec3(ea);
    return v[0].asS32() == vec[0] &&
           v[1].asS32() == vec[1] &&
           v[2].asS32() == vec[2];
}


static void begin_azel_vdp1_frame()
{
    // initVDP1() owns the first six setup commands in context 0. Gameplay/UI
    // commands are emitted after them. Rewind only the transient tail each
    // frame, matching the Saturn command-list lifecycle without asking the
    // desktop renderer to flush it.
    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    if (mainContextVdp1[0].size() < 1024)
        return;

    ctx.m0_currentVdp1WriteEA = mainContextVdp1[0].begin() + 6;
    ctx.m20_pCurrentVdp1Packet = ctx.m24_vdp1Packets;
    ctx.m1C = 0;
    ctx.mC = 0;
    ctx.m10 = ctx.m14[0].begin();
}

static void log_title_vdp2_diagnostics_once()
{
    static bool logged = false;
    if (logged)
        return;

    unsigned int banks[8] = {};
    unsigned int nonzeroPatterns = 0;
    // Title NBG0 uses two 0x800-byte 1-word pattern-name pages at 0x10000
    // and 0x10800. Inspect only metadata; rendering remains entirely SGX.
    for (unsigned int page = 0; page < 2; ++page) {
        const unsigned int base = 0x10000u + page * 0x800u;
        for (unsigned int i = 0; i < 0x400u; ++i) {
            const u16 pattern = getVdp2VramU16(base + i * 2u);
            if (pattern != 0)
                ++nonzeroPatterns;
            ++banks[(pattern >> 12) & 7u];
        }
    }

    // Avoid logging before TITLEE.PNB has actually been loaded.
    if (nonzeroPatterns == 0)
        return;

    lagi::platform::logging::writef(
        "[VDP2TitleDiag] patterns=%u banks=%u,%u,%u,%u,%u,%u,%u,%u "
        "cram0=%04X cram1=%04X cram2=%04X cram3=%04X\n",
        nonzeroPatterns,
        banks[0], banks[1], banks[2], banks[3],
        banks[4], banks[5], banks[6], banks[7],
        getVdp2CramU16(0),
        getVdp2CramU16(2),
        getVdp2CramU16(4),
        getVdp2CramU16(6));
    logged = true;
}

bool runtime_smoke_init()
{
    if (!saturn_memory_smoke_test()) {
        std::printf("[Azel] Saturn memory reader smoke test FAILED\n");
        lagi::platform::renderer::failure("[FAIL] SATURN MEMORY READERS");
        return false;
    }
    std::printf("[Azel] Saturn memory reader smoke test passed\n");

    if (!lagi::disc::init()) {
        std::printf("[Disc] no valid ISO9660 image found in ux0:data/lagi\n");
        lagi::platform::renderer::failure("[FAIL] DISC 1 CUE/BIN MOUNT");
        return false;
    }

    lagi::platform::renderer::set_disc_alive(true);

    // 0.040-alpha: enter through Azel's native startup path. Azel owns all
    // game/global initialization, VDP state, initial task creation, title
    // sequencing, New Game setup and subsequent game-status transitions.
    // Lagi provides only the Vita platform services underneath those calls.
    azelInit();
    resetEngine();

    // Native boot can enter the title movie immediately. Stop the CPU loading
    // framebuffer from overwriting GXM/movie presentation; front-end VDP2
    // composition will be added to this same native presentation path.
    lagi::platform::renderer::show_town_scene();

    lagi::platform::logging::writef(
        "[AzelBoot] native azelInit/resetEngine complete; initial task active\n");
    return true;
}
void runtime_smoke_frame()
{
    static unsigned int startupFrame = 0;
    const bool traceStartup = startupFrame < 3;

    // Present Vita controls as Azel's Saturn 3D pad. Input translation stays a
    // platform responsibility; task/menu/gameplay interpretation stays Azel's.
    auto& pending =
        graphicEngineStatus.m4514.m0_inputDevices[0].m16_pending;
    pending.m0_inputType = 2;
    pending.m6_buttonDown = lagi::platform::input::saturn_buttons_down();
    pending.m8_newButtonDown = lagi::platform::input::saturn_buttons_pressed();
    pending.mC_newButtonDown2 =
        lagi::platform::input::saturn_buttons_pressed();

    const float analogX = -lagi::platform::input::analog_x();
    const float analogY = -lagi::platform::input::analog_y();
    pending.m2_analogX = static_cast<s8>(
        analogX <= -1.0f ? -127 :
        analogX >= 1.0f ? 127 :
        analogX * 127.0f);
    pending.m3_analogY = static_cast<s8>(
        analogY <= -1.0f ? -127 :
        analogY >= 1.0f ? 127 :
        analogY * 127.0f);

    updateInputs();

    // Azel's Saturn VBlank normally advances fade state before the task pass.
    updateFadeInterrupt();
    {
        const auto& regs = vdp2Controls.m20_registers[0];
        lagi::platform::renderer::set_azel_color_offset_state(
            regs.m110_CLOFEN,
            regs.m112_CLOFSL,
            regs.m114_COAR,
            regs.m116_COAG,
            regs.m118_COAB,
            regs.m11A_COBR,
            regs.m11C_COBG,
            regs.m11E_COBB);
    }

    begin_azel_vdp1_frame();
    lagi::azel_bridge::begin_frame();

    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelBoot] frame=%u tasks=%d currentInitial=%p pendingInitial=%p\n",
            startupFrame,
            numActiveTask,
            reinterpret_cast<void*>(initialTaskStatus.m_currentTask),
            reinterpret_cast<void*>(initialTaskStatus.m_pendingTask));

    runTasks();

    // Service the Saturn-side VDP2 deferred register/DMA work and the platform
    // movie backend at the same host-frame boundary used by the existing
    // native runtime integration.
    interruptVDP2Update();
    lastUpdateFunction();

    // Front-end VDP2 presentation is a platform service. Azel owns all title
    // graphics, text, palettes, blinking, input and state transitions; Lagi
    // simply presents the VDP2 memory that Azel has already produced.
    const bool titleActive =
        initialTaskStatus.m_currentTask == createTitleScreenTask ||
        initialTaskStatus.m_currentTask == createTitleMenuTask;
    // Game status 2 is the complete FLD_D5 name-entry sequence, not merely
    // the keyboard. Azel deliberately keeps NBG0 disabled while field script 0
    // presents the opening background/dialogue, then nameEntryEnable() turns
    // NBG0 on when the keyboard is actually allowed to appear. Keep presenting
    // D5 throughout that sequence and carry NBG0 visibility separately.
    const bool d5NameSequenceActive =
        gGameStatus.m4_gameStatus == 2;
    const bool nameKeyboardVisible =
        d5NameSequenceActive &&
        (vdp2Controls.m4_pendingVdp2Regs->m20_BGON & 0x1) != 0;

    if (titleActive)
        log_title_vdp2_diagnostics_once();

    if (d5NameSequenceActive) {
        static bool loggedD5Vdp2 = false;
        if (!loggedD5Vdp2) {
            const auto* regs = vdp2Controls.m4_pendingVdp2Regs;
            const auto& a = gCoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
            const auto& b = gCoefficientTables[1][vdp2Controls.m0_doubleBufferIndex];
            lagi::platform::logging::writef(
                "[D5VDP2] BGON=%04X RPMD=%u PLSZ=%04X PNCR=%04X MPOFR=%04X "
                "KTCTL=%04X KTAOF=%04X WCTLC=%04X WCTLD=%04X LWTA1=%08X\n",
                regs->m20_BGON,
                static_cast<unsigned int>(regs->mB0_RPMD & 3u),
                regs->m3A_PLSZ, regs->m38_PNCR, regs->m3E_MPOFR,
                regs->mB4_KTCTL, regs->mB6_KTAOF,
                regs->mD4_WCTLC, regs->mD6_WCTLD,
                static_cast<unsigned int>(regs->mDC_LWTA1));
            lagi::platform::logging::writef(
                "[D5RBG0A] Xst=%08X Yst=%08X DXx=%08X DXy=%08X DYx=%08X DYy=%08X "
                "A=%08X B=%08X D=%08X E=%08X Px=%04X Py=%04X Cx=%04X Cy=%04X "
                "Mx=%08X My=%08X KAst=%08X DKAx=%08X DKAy=%08X\n",
                static_cast<unsigned int>(a.m0),
                static_cast<unsigned int>(a.m4),
                static_cast<unsigned int>(a.mC),
                static_cast<unsigned int>(a.m10),
                static_cast<unsigned int>(a.m14),
                static_cast<unsigned int>(a.m18),
                static_cast<unsigned int>(a.m1C),
                static_cast<unsigned int>(a.m20),
                static_cast<unsigned int>(a.m28),
                static_cast<unsigned int>(a.m2C),
                static_cast<unsigned int>(static_cast<u16>(a.m34)),
                static_cast<unsigned int>(static_cast<u16>(a.m36)),
                static_cast<unsigned int>(static_cast<u16>(a.m3C)),
                static_cast<unsigned int>(static_cast<u16>(a.m3E)),
                static_cast<unsigned int>(a.m44),
                static_cast<unsigned int>(a.m48),
                static_cast<unsigned int>(a.m54),
                static_cast<unsigned int>(a.m58),
                static_cast<unsigned int>(a.m5C));
            lagi::platform::logging::writef(
                "[D5RBG0B] Xst=%08X Yst=%08X DXx=%08X DXy=%08X DYx=%08X DYy=%08X "
                "A=%08X B=%08X D=%08X E=%08X Mx=%08X My=%08X KAst=%08X DKAx=%08X DKAy=%08X\n",
                static_cast<unsigned int>(b.m0),
                static_cast<unsigned int>(b.m4),
                static_cast<unsigned int>(b.mC),
                static_cast<unsigned int>(b.m10),
                static_cast<unsigned int>(b.m14),
                static_cast<unsigned int>(b.m18),
                static_cast<unsigned int>(b.m1C),
                static_cast<unsigned int>(b.m20),
                static_cast<unsigned int>(b.m28),
                static_cast<unsigned int>(b.m2C),
                static_cast<unsigned int>(b.m44),
                static_cast<unsigned int>(b.m48),
                static_cast<unsigned int>(b.m54),
                static_cast<unsigned int>(b.m58),
                static_cast<unsigned int>(b.m5C));
            loggedD5Vdp2 = true;
        }
    }

    if (d5NameSequenceActive) {
        // RBG0 is a 4x4 rotation map. D5 uses CHSZ=1 / PNB=1, so each
        // plane is one 0x800-byte page. Convert Azel's native MPOFR/MPxxRA
        // register encoding to byte offsets exactly as renderer_vdp2.cpp does.
        const auto* regs = vdp2Controls.m4_pendingVdp2Regs;
        const unsigned int pageSize = 0x800u;
        const unsigned int mapOffset =
            ((regs->m3E_MPOFR >> 0) & 7u) << 6;
        const u16 packed[8] = {
            regs->m50_MPABRA, regs->m52_MPCDRA,
            regs->m54_MPEFRA, regs->m56_MPGHRA,
            regs->m58_MPIJRA, regs->m5A_MPKLRA,
            regs->m5C_MPMNRA, regs->m5E_MPOPRA
        };
        unsigned int planeOffsets[16] = {};
        for (unsigned int i = 0; i < 8; ++i) {
            planeOffsets[i * 2 + 0] =
                (mapOffset + (packed[i] & 0x3Fu)) * pageSize;
            planeOffsets[i * 2 + 1] =
                (mapOffset + ((packed[i] >> 8) & 0x3Fu)) * pageSize;
        }
        lagi::platform::renderer::frontend_set_rbg0_planes(
            planeOffsets, 16u);
    }

    if (titleActive || d5NameSequenceActive) {
        // Layout 0 = title. Layout 2 = complete FLD_D5 name-entry sequence.
        // Bit 0 of flags is Azel's native NBG0 keyboard visibility.
        lagi::platform::renderer::frontend_present_vdp2(
            getVdp2Vram(0),
            getVdp2Cram(0),
            d5NameSequenceActive ? 2u : 0u,
            nameKeyboardVisible
                ? (vdp2Controls.m20_registers[0].m70_SCXN0 >> 16)
                : 0,
            nameKeyboardVisible
                ? (vdp2Controls.m20_registers[0].m74_SCYN0 >> 16)
                : 0,
            nameKeyboardVisible ? 1u : 0u);
    } else if (gGameStatus.m0_gameMode == 0 &&
               fileInfoStruct.mC_gfsHandle == nullptr) {
        // The native movie task closes its stream at the start of the exit
        // fade.  Re-submit the retained final frame so Azel's live color
        // offset continues to reach the display until the next module owns it.
        lagi::platform::renderer::movie_republish_frame();
    }

    ++startupFrame;
}

} // namespace lagi::azel
