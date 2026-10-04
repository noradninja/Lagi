#include <cstdio>
#include <psp2/kernel/processmgr.h>

#include "lagi/lagi_compat.h"
#include "lagi/platform.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_scene_bridge.h"
#include "lagi/lagi_input_bridge.h"
#include "lagi/lagi_diagnostics.h"
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
p_workArea createTitleMenuTask(p_workArea);

namespace lagi::azel {

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

static void capture_azel_vdp1_frontend_commands()
{
    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    if (mainContextVdp1[0].size() < 6)
        return;

    const auto begin = mainContextVdp1[0].begin() + 6;
    const auto end = ctx.m0_currentVdp1WriteEA;
    unsigned int commandCount = 0u;
    unsigned int normalSprites = 0u;
    unsigned int scaledSprites = 0u;
    unsigned int distortedSprites = 0u;
    unsigned int polylines = 0u;
    unsigned int otherCommands = 0u;
    std::uint32_t signature = 2166136261u;

    for (auto cmd = begin; cmd != end; ++cmd) {
        lagi::azel_bridge::Vdp1UiCommand ui{};
        ui.cmdCtrl = cmd->m0_CMDCTRL;
        ui.cmdPmod = cmd->m4_CMDPMOD;
        ui.cmdColr = cmd->m6_CMDCOLR;
        ui.cmdSrca = cmd->m8_CMDSRCA;
        ui.cmdSize = cmd->mA_CMDSIZE;
        ui.xa = cmd->mC_CMDXA;   ui.ya = cmd->mE_CMDYA;
        ui.xb = cmd->m10_CMDXB;  ui.yb = cmd->m12_CMDYB;
        ui.xc = cmd->m14_CMDXC;  ui.yc = cmd->m16_CMDYC;
        ui.xd = cmd->m18_CMDXD;  ui.yd = cmd->m1A_CMDYD;
        lagi::azel_bridge::record_vdp1_ui_command(ui);

        ++commandCount;
        switch (ui.cmdCtrl & 0x000Fu) {
        case 0x0u: ++normalSprites; break;
        case 0x1u: ++scaledSprites; break;
        case 0x2u: ++distortedSprites; break;
        case 0x5u: ++polylines; break;
        default: ++otherCommands; break;
        }

        const std::uint16_t words[] = {
            ui.cmdCtrl, ui.cmdPmod, ui.cmdColr, ui.cmdSrca, ui.cmdSize,
            static_cast<std::uint16_t>(ui.xa),
            static_cast<std::uint16_t>(ui.ya),
            static_cast<std::uint16_t>(ui.xb),
            static_cast<std::uint16_t>(ui.yb)
        };
        for (const std::uint16_t word : words) {
            signature ^= static_cast<std::uint8_t>(word & 0xFFu);
            signature *= 16777619u;
            signature ^= static_cast<std::uint8_t>(word >> 8);
            signature *= 16777619u;
        }
    }

    static std::uint32_t lastSignature = 0u;
    static unsigned int heartbeat = 0u;
    if (signature != lastSignature || ((heartbeat++ % 120u) == 0u)) {
        lagi::platform::logging::writef(
            "[PresentationTrace][AzelVDP1] mode=%d status=%d cmds=%u "
            "normal=%u scaled=%u distorted=%u polyline=%u other=%u hash=%08X\n",
            static_cast<int>(gGameStatus.m0_gameMode),
            static_cast<int>(gGameStatus.m4_gameStatus),
            commandCount,
            normalSprites,
            scaledSprites,
            distortedSprites,
            polylines,
            otherCommands,
            static_cast<unsigned int>(signature));
        lastSignature = signature;
    }

    if (gGameStatus.m0_gameMode == 1 && commandCount != 0u) {
        static unsigned int detailBudget = 48u;
        if (detailBudget != 0u) {
            unsigned int index = 0u;
            for (auto cmd = begin;
                 cmd != end && index < 8u && detailBudget != 0u;
                 ++cmd, ++index, --detailBudget) {
                lagi::platform::logging::writef(
                    "[PresentationTrace][AzelVDP1Cmd] i=%u CTRL=%04X PMOD=%04X "
                    "COLR=%04X SRCA=%04X SIZE=%04X "
                    "A=(%d,%d) B=(%d,%d) C=(%d,%d) D=(%d,%d)\n",
                    index,
                    cmd->m0_CMDCTRL,
                    cmd->m4_CMDPMOD,
                    cmd->m6_CMDCOLR,
                    cmd->m8_CMDSRCA,
                    cmd->mA_CMDSIZE,
                    cmd->mC_CMDXA, cmd->mE_CMDYA,
                    cmd->m10_CMDXB, cmd->m12_CMDYB,
                    cmd->m14_CMDXC, cmd->m16_CMDYC,
                    cmd->m18_CMDXD, cmd->m1A_CMDYD);
            }
        }
    }
}

static void build_rbg0_gpu_parameter(
    const sCoefficientTableData& t,
    float transform[8],
    float coefficient[4])
{
    auto truncFP = [](s32 v) -> s32 {
        return v & static_cast<s32>(0xFFFFFFC0u);
    };
    auto signExt14fp = [](s16 v) -> s32 {
        s32 x = static_cast<s32>(v) & 0x3FFF;
        if (x & 0x2000)
            x |= static_cast<s32>(0xFFFFC000u);
        return x << 16;
    };
    auto truncMx = [](s32 v) -> s32 {
        return (v & 0x3FFFFFC0) |
            ((v & 0x20000000) ? static_cast<s32>(0xE0000000u) : 0);
    };
    auto fpMul = [](s32 a, s32 b) -> s32 {
        return static_cast<s32>(
            (static_cast<long long>(a) * static_cast<long long>(b)) >> 16);
    };
    auto fp = [](s32 v) -> float {
        return static_cast<float>(v) / 65536.0f;
    };

    const s32 A = truncFP(t.m1C);
    const s32 B = truncFP(t.m20);
    const s32 C = truncFP(t.m24);
    const s32 D = truncFP(t.m28);
    const s32 E = truncFP(t.m2C);
    const s32 F = truncFP(t.m30);

    const s32 Px = signExt14fp(t.m34);
    const s32 Py = signExt14fp(t.m36);
    const s32 Pz = signExt14fp(t.m38);
    const s32 Cx = signExt14fp(t.m3C);
    const s32 Cy = signExt14fp(t.m3E);
    const s32 Cz = signExt14fp(t.m40);

    const s32 Xp =
        fpMul(A, Px - Cx) + fpMul(B, Py - Cy) +
        fpMul(C, Pz - Cz) + Cx + truncMx(t.m44);
    const s32 Yp =
        fpMul(D, Px - Cx) + fpMul(E, Py - Cy) +
        fpMul(F, Pz - Cz) + Cy + truncMx(t.m48);

    const s32 xmul = truncFP(t.m0) - Px;
    const s32 ymul = truncFP(t.m4) - Py;
    const s32 zrel = truncFP(t.m8_Zst) - Pz;

    const s32 xBase =
        fpMul(A, xmul) + fpMul(B, ymul) + fpMul(C, zrel);
    const s32 yBase =
        fpMul(D, xmul) + fpMul(E, ymul) + fpMul(F, zrel);

    const s32 xStep =
        fpMul(A, truncFP(t.m14)) + fpMul(B, truncFP(t.m18));
    const s32 yStep =
        fpMul(D, truncFP(t.m14)) + fpMul(E, truncFP(t.m18));

    const s32 xYStep =
        fpMul(A, truncFP(t.mC)) + fpMul(B, truncFP(t.m10));
    const s32 yYStep =
        fpMul(D, truncFP(t.mC)) + fpMul(E, truncFP(t.m10));

    transform[0] = fp(xBase);
    transform[1] = fp(yBase);
    transform[2] = fp(xStep);
    transform[3] = fp(yStep);
    transform[4] = fp(xYStep);
    transform[5] = fp(yYStep);
    transform[6] = fp(Xp);
    transform[7] = fp(Yp);

    // KAst is consumed as an unsigned 32-bit fixed-point accumulator by
    // Azel's RBG0 renderer before the >>16 table index extraction. Preserve
    // that wrap domain here instead of interpreting 0x80000000 as -32768.
    coefficient[0] =
        static_cast<float>(static_cast<u32>(t.m54)) / 65536.0f;
    coefficient[1] = fp(truncFP(t.m58));
    coefficient[2] = fp(truncFP(t.m5C));
    coefficient[3] = 0.0f;
}

bool runtime_init()
{
    if (!lagi::diagnostics::saturn_memory_readers()) {
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
    lagi::platform::renderer::show_game_presentation();

    lagi::platform::logging::writef(
        "[AzelBoot] native azelInit/resetEngine complete; initial task active\n");
    return true;
}
void runtime_frame()
{
    static unsigned int startupFrame = 0;
    const bool traceStartup = startupFrame < 3;

    lagi::input_bridge::sync_to_azel();

    // Azel's Saturn VBlank normally advances fade state before the task pass.
    updateFadeInterrupt();

    lagi::diagnostics::trace_fade_state();

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

    // Mode 1 is currently the first native 3D scene class Neptune can
    // present. This is a capability check, not Lagi ownership of "town".
    const bool nativeSceneFrame =
        gGameStatus.m0_gameMode == 1;
    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelBoot] frame=%u tasks=%d currentInitial=%p pendingInitial=%p\n",
            startupFrame,
            numActiveTask,
            reinterpret_cast<void*>(initialTaskStatus.m_currentTask),
            reinterpret_cast<void*>(initialTaskStatus.m_pendingTask));

    runTasks();

    // Capture the VDP1 commands Azel emitted this frame before the transient
    // command tail is rewound on the next host frame. Neptune publishes this
    // snapshot only after it owns the front-end render slot.
    capture_azel_vdp1_frontend_commands();

    if (nativeSceneFrame) {
        // Azel owns the active scene and all of its task/gameplay state.
        // Lagi snapshots only renderer-facing state through the generic scene
        // bridge after the native task pass.
        lagi::scene_bridge::sync_presentation_state();
    }

    if (gGameStatus.m4_gameStatus == 2 &&
        (vdp2Controls.m4_pendingVdp2Regs->m20_BGON & 0x1u) != 0) {
        static unsigned int d5Vdp1DiagFrames = 0;
        if (d5Vdp1DiagFrames < 8u) {
            // The current frame is not published until frontend_present_vdp2(),
            // so inspect the live command buffer directly through the capture
            // count by walking the native VDP1 tail here.
            auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
            const auto begin = mainContextVdp1[0].begin() + 6;
            const auto end = ctx.m0_currentVdp1WriteEA;
            unsigned int count = 0;
            for (auto cmd = begin; cmd != end && count < 12u; ++cmd, ++count) {
                lagi::platform::logging::writef(
                    "[D5VDP1] f=%u i=%u CTRL=%04X PMOD=%04X COLR=%04X "
                    "SRCA=%04X SIZE=%04X A=(%d,%d) B=(%d,%d) "
                    "C=(%d,%d) D=(%d,%d)\n",
                    d5Vdp1DiagFrames, count,
                    cmd->m0_CMDCTRL, cmd->m4_CMDPMOD,
                    cmd->m6_CMDCOLR, cmd->m8_CMDSRCA,
                    cmd->mA_CMDSIZE,
                    cmd->mC_CMDXA, cmd->mE_CMDYA,
                    cmd->m10_CMDXB, cmd->m12_CMDYB,
                    cmd->m14_CMDXC, cmd->m16_CMDYC,
                    cmd->m18_CMDXD, cmd->m1A_CMDYD);
            }
            ++d5Vdp1DiagFrames;
        }
    }

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
        lagi::diagnostics::log_title_vdp2_once();

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
        // Snapshot the RBG0 control surface that Azel already produced.
        // Neptune consumes this as renderer state only; all map selection,
        // coefficient generation, windows and sequencing remain Azel-owned.
        const auto* regs = vdp2Controls.m4_pendingVdp2Regs;
        lagi::platform::renderer::FrontendRbg0State state{};

        // D5 config is CHSZ=1 / PNB=1, therefore each 4x4 rotation-map
        // plane occupies one 0x800-byte page. Derive both parameter A and B
        // maps exactly as renderer_vdp2.cpp does from MPOFR + MPxxR[A/B].
        constexpr unsigned int pageSize = 0x800u;
        const unsigned int mapOffsetA =
            ((regs->m3E_MPOFR >> 0) & 7u) << 6;
        const unsigned int mapOffsetB =
            ((regs->m3E_MPOFR >> 4) & 7u) << 6;
        const u16 packedA[8] = {
            regs->m50_MPABRA, regs->m52_MPCDRA,
            regs->m54_MPEFRA, regs->m56_MPGHRA,
            regs->m58_MPIJRA, regs->m5A_MPKLRA,
            regs->m5C_MPMNRA, regs->m5E_MPOPRA
        };
        const u16 packedB[8] = {
            regs->m60_MPABRB, regs->m62_MPCDRB,
            regs->m64_MPEFRB, regs->m66_MPGHRB,
            regs->m68_MPIJRB, regs->m6A_MPKLRB,
            regs->m6C_MPMNRB, regs->m6E_MPOPRB
        };
        for (unsigned int i = 0; i < 8; ++i) {
            state.planeA[i * 2 + 0] =
                (mapOffsetA + (packedA[i] & 0x3Fu)) * pageSize;
            state.planeA[i * 2 + 1] =
                (mapOffsetA + ((packedA[i] >> 8) & 0x3Fu)) * pageSize;
            state.planeB[i * 2 + 0] =
                (mapOffsetB + (packedB[i] & 0x3Fu)) * pageSize;
            state.planeB[i * 2 + 1] =
                (mapOffsetB + ((packedB[i] >> 8) & 0x3Fu)) * pageSize;
        }

        state.rpmd = regs->mB0_RPMD & 3u;
        state.ktctl = regs->mB4_KTCTL;
        state.ktaof = regs->mB6_KTAOF;
        state.wctlc = regs->mD4_WCTLC;
        state.wctld = regs->mD6_WCTLD;

        state.window0[0] = (regs->mC0_WPSX0 >> 1) & 0x1FF;
        state.window0[1] = regs->mC2_WPSY0 & 0x1FF;
        state.window0[2] = (regs->mC4_WPEX0 >> 1) & 0x1FF;
        state.window0[3] = regs->mC6_WPEY0 & 0x1FF;
        state.window1[0] = (regs->mC8_WPSX1 >> 1) & 0x1FF;
        state.window1[1] = regs->mCA_WPSY1 & 0x1FF;
        state.window1[2] = (regs->mCC_WPEX1 >> 1) & 0x1FF;
        state.window1[3] = regs->mCE_WPEY1 & 0x1FF;

        if (regs->mD8_LWTA0 & 0x80000000u) {
            state.lineWindowMask |= 1u;
            state.lineWindow0Address =
                ((regs->mD8_LWTA0 & 0x7FFFEu) << 1) & 0x7FFFFu;
        }
        if (regs->mDC_LWTA1 & 0x80000000u) {
            state.lineWindowMask |= 2u;
            state.lineWindow1Address =
                ((regs->mDC_LWTA1 & 0x7FFFEu) << 1) & 0x7FFFFu;
        }

        const auto& paramA =
            gCoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
        const auto& paramB =
            gCoefficientTables[1][vdp2Controls.m0_doubleBufferIndex];
        build_rbg0_gpu_parameter(
            paramA, state.transformA, state.coefficientA);
        build_rbg0_gpu_parameter(
            paramB, state.transformB, state.coefficientB);

        lagi::platform::renderer::frontend_set_rbg0_state(state);
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
            nameKeyboardVisible ? 1u : 0u,
            static_cast<unsigned int>(
                vdp2Controls.m20_registers[0].m0_TVMD));
    } else if (gGameStatus.m0_gameMode == 0 &&
               fileInfoStruct.mC_gfsHandle == nullptr) {
        // The native movie task closes its stream at the start of the exit
        // fade. Re-submit the retained final frame only while the movie-mode
        // state machine still owns presentation.
        lagi::platform::renderer::movie_republish_frame();
    } else if (gGameStatus.m0_gameMode == 1) {
        // The frame that *enters* native scene mode began as a movie/module-manager
        // frame and therefore did not acquire the scene producer slot. Release
        // the retained movie immediately, but publish only on a frame that
        // began in native scene mode and acquired that slot before runTasks().
        lagi::platform::renderer::movie_clear_frame();
        if (nativeSceneFrame) {
            // All scene adapters write staging state. Acquire the render slot
            // only at the publish boundary so a scene->movie/module transition
            // inside runTasks() cannot deadlock against movie presentation.
            lagi::platform::renderer::presentation_wait_frame_slot();
            lagi::azel_bridge::publish_frame();
            lagi::platform::renderer::presentation_publish_frame();

            static unsigned int sceneHeartbeat = 0;
            if ((sceneHeartbeat++ % 60u) == 0u) {
                lagi::platform::logging::writef(
                    "[AzelScene] frame heartbeat tasks=%d submissions=%u\n",
                    numActiveTask,
                    static_cast<unsigned int>(
                        lagi::azel_bridge::published_submissions().size()));
            }
        }
    } else if (gGameStatus.m0_gameMode != 0) {
        // Other native gameplay modes are not yet presented by Neptune.
        lagi::platform::renderer::movie_clear_frame();
    }

    ++startupFrame;
}

} // namespace lagi::azel
