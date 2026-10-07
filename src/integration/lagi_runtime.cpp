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
#include "audio/soundDriver.h"

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
namespace {

void correct_reconstructed_fade_step(sFadeControlsChannel& channel)
{
    if (channel.m20_stopped != 0)
        return;

    bool corrected = false;
    for (unsigned int i = 0; i < 3u; ++i) {
        const std::int32_t current = channel.m0_color[i].asS32();
        const std::int32_t target =
            static_cast<std::int32_t>(channel.m18_targetColor[i]) << 16;
        const std::int32_t step = channel.mC_colorStep[i].asS32();
        const std::int64_t delta =
            static_cast<std::int64_t>(target) -
            static_cast<std::int64_t>(current);

        // Upstream Azel currently reconstructs fadePalette() as
        //     step = (current - target) / frames
        // while updateFadeInterrupt() advances with
        //     current += step.
        // That walks away from the requested Saturn target until the final
        // frame snaps to it. Preserve Azel's target and timing, but correct
        // the reconstructed step direction at the platform boundary.
        if ((delta > 0 && step < 0) || (delta < 0 && step > 0)) {
            channel.mC_colorStep[i] =
                fixedPoint::fromS32(-step);
            corrected = true;
        }
    }

    if (corrected) {
        platform::logging::writef(
            "[FadeCompat] corrected reconstructed fade step direction\n");
    }
}

void correct_reconstructed_fade_steps()
{
    correct_reconstructed_fade_step(g_fadeControls.m0_fade0);
    correct_reconstructed_fade_step(g_fadeControls.m24_fade1);
}

} // namespace

void runtime_frame()
{
    static unsigned int startupFrame = 0;
    const bool traceStartup = startupFrame < 3;

    lagi::input_bridge::sync_to_azel();

    // Azel's Saturn VBlank normally advances fade state before the task pass.
    // Correct the sign error in the current host reconstruction before the
    // channel is advanced; Azel still owns the target color and frame count.
    correct_reconstructed_fade_steps();
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
    lagi::azel_bridge::begin_frame(gGameStatus.m0_gameMode == 3);

    // Native 3D scene capability check. Town (1) and field (3) both
    // remain Azel-owned; Lagi only opens their generic presentation boundary.
    const bool nativeSceneFrame =
        gGameStatus.m0_gameMode == 1 ||
        gGameStatus.m0_gameMode == 3;
    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelBoot] frame=%u tasks=%d currentInitial=%p pendingInitial=%p\n",
            startupFrame,
            numActiveTask,
            reinterpret_cast<void*>(initialTaskStatus.m_currentTask),
            reinterpret_cast<void*>(initialTaskStatus.m_pendingTask));

    runTasks();

    // Flight Phase 1: record only the transition into and activity around
    // game status 0x50 / game mode 3. This is diagnostic only; field
    // presentation remains disabled until the next milestone.
    {
        static int lastFlightStatus = -1;
        static int lastFlightMode = -1;
        static int lastFlightNext = -1;
        const int status = gGameStatus.m4_gameStatus;
        const int mode = gGameStatus.m0_gameMode;
        const int next = gGameStatus.m8_nextGameStatus;
        const bool flightRelevant =
            status == 5 || status == 0x50 || next == 0x50 || mode == 3;
        if (flightRelevant &&
            (status != lastFlightStatus ||
             mode != lastFlightMode ||
             next != lastFlightNext)) {
            lagi::platform::logging::writef(
                "[LagiFlight] status=%02X mode=%d next=%02X prev=%02X tasks=%d\n",
                static_cast<unsigned>(status),
                mode,
                static_cast<unsigned>(next),
                static_cast<unsigned>(gGameStatus.m6_previousGameStatus),
                numActiveTask);
            lastFlightStatus = status;
            lastFlightMode = mode;
            lastFlightNext = next;
        }
    }

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

    // Preserve the upstream PDS frame ordering for audio. The Vita runtime
    // owns the host loop, so service Azel's sound driver here at the same
    // post-task boundary where upstream PDS.cpp calls updateSound().
    updateSound();

    // Service the Saturn-side VDP2 deferred register/DMA work and the platform
    // movie backend at the same host-frame boundary used by the existing
    // native runtime integration.
    interruptVDP2Update();
    lastUpdateFunction();

    // Tasks above can start a new fade and change CLOFEN/CLOFSL as part of
    // the same VDP2 update that makes new front-end artwork visible. Refresh
    // Neptune from the completed Saturn register state here as well as at the
    // start of the frame, so a newly enabled layer cannot appear for one
    // frame without its color offset (notably the title NBG1 handoff).
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
        // PLSZ packs NBG0/1/2/3/RBG0 plane sizes in successive 2-bit
        // fields. RBG0 is bits 9:8; the low bits belong to NBG0.
        state.plsz = (regs->m3A_PLSZ >> 8) & 3u;
        state.chctlb = regs->m2A_CHCTLB;
        state.pncr = regs->m38_PNCR;
        state.craofb = regs->mE6_CRAOFB;
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

        static bool loggedD5RbgBridge = false;
        if (!loggedD5RbgBridge) {
            lagi::platform::logging::writef(
                "[D5RBGBridge] WCTLC=%04X WCTLD=%04X "
                "LWTA0=%08X LWTA1=%08X mask=%u "
                "addr0=%05X addr1=%05X "
                "W0=(%d,%d)-(%d,%d) W1=(%d,%d)-(%d,%d) "
                "RBGfmt CHCTLB=%04X PNCR=%04X CRAOFB=%04X PLSZ=%u\n",
                static_cast<unsigned int>(regs->mD4_WCTLC),
                static_cast<unsigned int>(regs->mD6_WCTLD),
                static_cast<unsigned int>(regs->mD8_LWTA0),
                static_cast<unsigned int>(regs->mDC_LWTA1),
                state.lineWindowMask,
                state.lineWindow0Address,
                state.lineWindow1Address,
                state.window0[0], state.window0[1],
                state.window0[2], state.window0[3],
                state.window1[0], state.window1[1],
                state.window1[2], state.window1[3],
                state.chctlb, state.pncr, state.craofb, state.plsz);

            const unsigned char* liveVram = getVdp2Vram(0);
            const unsigned int sampleY[] = {
                0u, 32u, 64u, 96u, 112u, 128u, 160u, 192u, 223u
            };
            if ((state.lineWindowMask & 2u) != 0u) {
                for (unsigned int si = 0;
                     si < sizeof(sampleY) / sizeof(sampleY[0]); ++si) {
                    const unsigned int y = sampleY[si];
                    const unsigned int addr =
                        (state.lineWindow1Address + y * 4u) & 0x7FFFFu;
                    const unsigned int xs =
                        static_cast<unsigned int>(liveVram[addr]) |
                        (static_cast<unsigned int>(
                            liveVram[(addr + 1u) & 0x7FFFFu]) << 8);
                    const unsigned int xe =
                        static_cast<unsigned int>(
                            liveVram[(addr + 2u) & 0x7FFFFu]) |
                        (static_cast<unsigned int>(
                            liveVram[(addr + 3u) & 0x7FFFFu]) << 8);
                    lagi::platform::logging::writef(
                        "[D5RBGBridge] LW1 y=%u raw=%04X..%04X "
                        "x=%u..%u\n",
                        y, xs, xe,
                        (xs >> 1) & 0x1FFu,
                        (xe >> 1) & 0x1FFu);
                }
            }
            loggedD5RbgBridge = true;
        }

        const auto& paramA =
            gCoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
        const auto& paramB =
            gCoefficientTables[1][vdp2Controls.m0_doubleBufferIndex];
        build_rbg0_gpu_parameter(
            paramA, state.transformA, state.coefficientA);
        build_rbg0_gpu_parameter(
            paramB, state.transformB, state.coefficientB);

        static bool loggedD5RbgParameterB = false;
        if (!loggedD5RbgParameterB) {
            lagi::platform::logging::writef(
                "[D5RBGPlanes] A=%05X,%05X,%05X,%05X "
                "B=%05X,%05X,%05X,%05X\n",
                state.planeA[0], state.planeA[1],
                state.planeA[2], state.planeA[3],
                state.planeB[0], state.planeB[1],
                state.planeB[2], state.planeB[3]);

            lagi::platform::logging::writef(
                "[D5RBGXformA] T0=%f,%f,%f,%f T1=%f,%f,%f,%f "
                "K=%f,%f,%f\n",
                state.transformA[0], state.transformA[1],
                state.transformA[2], state.transformA[3],
                state.transformA[4], state.transformA[5],
                state.transformA[6], state.transformA[7],
                state.coefficientA[0], state.coefficientA[1],
                state.coefficientA[2]);
            lagi::platform::logging::writef(
                "[D5RBGXformB] T0=%f,%f,%f,%f T1=%f,%f,%f,%f "
                "K=%f,%f,%f\n",
                state.transformB[0], state.transformB[1],
                state.transformB[2], state.transformB[3],
                state.transformB[4], state.transformB[5],
                state.transformB[6], state.transformB[7],
                state.coefficientB[0], state.coefficientB[1],
                state.coefficientB[2]);

            const unsigned char* liveVram = getVdp2Vram(0);
            const unsigned int coefficientSizeB =
                (state.ktctl & 0x200u) ? 2u : 4u;
            const unsigned int coefficientBaseB =
                ((state.ktaof >> 8) & 0x7u) *
                coefficientSizeB * 0x10000u;

            const unsigned int ys[] = {112u, 128u, 160u, 192u, 223u};
            const unsigned int xs[] = {0u, 176u, 351u};

            for (unsigned int yi = 0;
                 yi < sizeof(ys) / sizeof(ys[0]); ++yi) {
                for (unsigned int xi = 0;
                     xi < sizeof(xs) / sizeof(xs[0]); ++xi) {
                    const unsigned int y = ys[yi];
                    const unsigned int x = xs[xi];

                    const std::uint32_t accum =
                        static_cast<std::uint32_t>(paramB.m54) +
                        static_cast<std::uint32_t>(paramB.m58) * y +
                        static_cast<std::uint32_t>(paramB.m5C) * x;
                    const unsigned int index = accum >> 16;
                    const unsigned int addr =
                        (coefficientBaseB +
                         index * coefficientSizeB) & 0x7FFFFu;

                    const std::uint32_t raw =
                        static_cast<std::uint32_t>(liveVram[addr]) |
                        (static_cast<std::uint32_t>(
                            liveVram[(addr + 1u) & 0x7FFFFu]) << 8) |
                        (static_cast<std::uint32_t>(
                            liveVram[(addr + 2u) & 0x7FFFFu]) << 16) |
                        (static_cast<std::uint32_t>(
                            liveVram[(addr + 3u) & 0x7FFFFu]) << 24);

                    std::int32_t k =
                        static_cast<std::int32_t>(raw & 0x00FFFFFFu);
                    if ((k & 0x00800000) != 0)
                        k |= static_cast<std::int32_t>(0xFF000000u);

                    lagi::platform::logging::writef(
                        "[D5RBGCoeffB] xy=%u,%u accum=%08X "
                        "idx=%04X addr=%05X raw=%08X k=%f\n",
                        x, y, accum, index, addr, raw,
                        static_cast<float>(k) / 65536.0f);

                    const unsigned int noOffsetAddr =
                        (index * coefficientSizeB) & 0x7FFFFu;
                    const std::uint32_t rawNoOffset =
                        static_cast<std::uint32_t>(
                            liveVram[noOffsetAddr]) |
                        (static_cast<std::uint32_t>(
                            liveVram[(noOffsetAddr + 1u) & 0x7FFFFu]) << 8) |
                        (static_cast<std::uint32_t>(
                            liveVram[(noOffsetAddr + 2u) & 0x7FFFFu]) << 16) |
                        (static_cast<std::uint32_t>(
                            liveVram[(noOffsetAddr + 3u) & 0x7FFFFu]) << 24);
                    std::int32_t kNoOffset =
                        static_cast<std::int32_t>(
                            rawNoOffset & 0x00FFFFFFu);
                    if ((kNoOffset & 0x00800000) != 0)
                        kNoOffset |=
                            static_cast<std::int32_t>(0xFF000000u);
                    lagi::platform::logging::writef(
                        "[D5RBGCoeffB0] xy=%u,%u addr=%05X "
                        "raw=%08X k=%f\n",
                        x, y, noOffsetAddr, rawNoOffset,
                        static_cast<float>(kNoOffset) / 65536.0f);
                }
            }

            // Compare Azel's exact signed 16.16 coordinate pipeline
            // against the float form sent to Neptune at representative B
            // pixels. This isolates transform precision/order from tile
            // decoding and window selection.
            auto truncFPDiag = [](s32 v) -> s32 {
                return v & static_cast<s32>(0xFFFFFFC0u);
            };
            auto signExt14fpDiag = [](s16 v) -> s32 {
                s32 x = static_cast<s32>(v) & 0x3FFF;
                if (x & 0x2000)
                    x |= static_cast<s32>(0xFFFFC000u);
                return x << 16;
            };
            auto truncMxDiag = [](s32 v) -> s32 {
                return (v & 0x3FFFFFC0) |
                    ((v & 0x20000000) ?
                        static_cast<s32>(0xE0000000u) : 0);
            };
            auto fpMulDiag = [](s32 a, s32 b) -> s32 {
                return static_cast<s32>(
                    (static_cast<long long>(a) *
                     static_cast<long long>(b)) >> 16);
            };

            const s32 A_Bd = truncFPDiag(paramB.m1C);
            const s32 B_Bd = truncFPDiag(paramB.m20);
            const s32 C_Bd = truncFPDiag(paramB.m24);
            const s32 D_Bd = truncFPDiag(paramB.m28);
            const s32 E_Bd = truncFPDiag(paramB.m2C);
            const s32 F_Bd = truncFPDiag(paramB.m30);
            const s32 Px_Bd = signExt14fpDiag(paramB.m34);
            const s32 Py_Bd = signExt14fpDiag(paramB.m36);
            const s32 Pz_Bd = signExt14fpDiag(paramB.m38);
            const s32 Cx_Bd = signExt14fpDiag(paramB.m3C);
            const s32 Cy_Bd = signExt14fpDiag(paramB.m3E);
            const s32 Cz_Bd = signExt14fpDiag(paramB.m40);
            const s32 Xp_Bd =
                fpMulDiag(A_Bd, Px_Bd - Cx_Bd) +
                fpMulDiag(B_Bd, Py_Bd - Cy_Bd) +
                fpMulDiag(C_Bd, Pz_Bd - Cz_Bd) +
                Cx_Bd + truncMxDiag(paramB.m44);
            const s32 Yp_Bd =
                fpMulDiag(D_Bd, Px_Bd - Cx_Bd) +
                fpMulDiag(E_Bd, Py_Bd - Cy_Bd) +
                fpMulDiag(F_Bd, Pz_Bd - Cz_Bd) +
                Cy_Bd + truncMxDiag(paramB.m48);
            const s32 baseXmul_Bd =
                truncFPDiag(paramB.m0) - Px_Bd;
            const s32 baseYmul_Bd =
                truncFPDiag(paramB.m4) - Py_Bd;
            const s32 zrel_Bd =
                truncFPDiag(paramB.m8_Zst) - Pz_Bd;
            const s32 Cval_Bd = fpMulDiag(C_Bd, zrel_Bd);
            const s32 Fval_Bd = fpMulDiag(F_Bd, zrel_Bd);
            const s32 dX_Bd =
                fpMulDiag(A_Bd, truncFPDiag(paramB.m14)) +
                fpMulDiag(B_Bd, truncFPDiag(paramB.m18));
            const s32 dY_Bd =
                fpMulDiag(D_Bd, truncFPDiag(paramB.m14)) +
                fpMulDiag(E_Bd, truncFPDiag(paramB.m18));
            const s32 dXst_Bd = truncFPDiag(paramB.mC);
            const s32 dYst_Bd = truncFPDiag(paramB.m10);
            const s32 dKAst_Bd = truncFPDiag(paramB.m58);
            const s32 dKAx_Bd = truncFPDiag(paramB.m5C);
            const s32 KAst_Bd = truncFPDiag(paramB.m54);

            const unsigned int testYs[] = {112u, 128u, 160u, 192u, 223u};
            const unsigned int testXs[] = {0u, 176u, 351u};
            for (unsigned int yi = 0;
                 yi < sizeof(testYs) / sizeof(testYs[0]); ++yi) {
                const unsigned int y = testYs[yi];
                const s32 xmulLine =
                    baseXmul_Bd + static_cast<s32>(
                        static_cast<std::int64_t>(dXst_Bd) * y);
                const s32 ymulLine =
                    baseYmul_Bd + static_cast<s32>(
                        static_cast<std::int64_t>(dYst_Bd) * y);
                const s32 Xsp =
                    fpMulDiag(A_Bd, xmulLine) +
                    fpMulDiag(B_Bd, ymulLine) + Cval_Bd;
                const s32 Ysp =
                    fpMulDiag(D_Bd, xmulLine) +
                    fpMulDiag(E_Bd, ymulLine) + Fval_Bd;

                for (unsigned int xi = 0;
                     xi < sizeof(testXs) / sizeof(testXs[0]); ++xi) {
                    const unsigned int x = testXs[xi];

                    const std::uint32_t accum =
                        static_cast<std::uint32_t>(KAst_Bd) +
                        static_cast<std::uint32_t>(dKAst_Bd) * y +
                        static_cast<std::uint32_t>(dKAx_Bd) * x;
                    const unsigned int coeffIndex = accum >> 16;
                    const unsigned int coeffAddr =
                        (coeffIndex * 4u) & 0x7FFFFu;
                    const std::uint32_t rawCoeff =
                        static_cast<std::uint32_t>(
                            liveVram[coeffAddr]) |
                        (static_cast<std::uint32_t>(
                            liveVram[(coeffAddr + 1u) & 0x7FFFFu]) << 8) |
                        (static_cast<std::uint32_t>(
                            liveVram[(coeffAddr + 2u) & 0x7FFFFu]) << 16) |
                        (static_cast<std::uint32_t>(
                            liveVram[(coeffAddr + 3u) & 0x7FFFFu]) << 24);
                    s32 kx =
                        static_cast<s32>(rawCoeff & 0x00FFFFFFu);
                    if (kx & 0x00800000)
                        kx |= static_cast<s32>(0xFF000000u);

                    const s32 XspPixel =
                        Xsp + fpMulDiag(
                            dX_Bd, static_cast<s32>(x << 16));
                    const s32 YspPixel =
                        Ysp + fpMulDiag(
                            dY_Bd, static_cast<s32>(x << 16));
                    const s32 rawX =
                        fpMulDiag(kx, XspPixel) + Xp_Bd;
                    const s32 rawY =
                        fpMulDiag(kx, YspPixel) + Yp_Bd;
                    const int exactX = (rawX >> 16) & 0x7FF;
                    const int exactY = (rawY >> 16) & 0x7FF;

                    const float kf =
                        static_cast<float>(kx) / 65536.0f;
                    const float floatRotX =
                        state.transformB[0] +
                        state.transformB[4] * static_cast<float>(y) +
                        state.transformB[2] * static_cast<float>(x);
                    const float floatRotY =
                        state.transformB[1] +
                        state.transformB[5] * static_cast<float>(y) +
                        state.transformB[3] * static_cast<float>(x);
                    const float floatMapX =
                        kf * floatRotX + state.transformB[6];
                    const float floatMapY =
                        kf * floatRotY + state.transformB[7];
                    int approxX =
                        static_cast<int>(std::floor(floatMapX)) & 0x7FF;
                    int approxY =
                        static_cast<int>(std::floor(floatMapY)) & 0x7FF;

                    lagi::platform::logging::writef(
                        "[D5RBGCoordB] xy=%u,%u idx=%04X k=%f "
                        "exact=%d,%d float=%d,%d delta=%d,%d\n",
                        x, y, coeffIndex,
                        static_cast<float>(kx) / 65536.0f,
                        exactX, exactY,
                        approxX, approxY,
                        approxX - exactX,
                        approxY - exactY);

                    // Mirror sampleTileAtCoordinate() for this D5 format:
                    // CHSZ=1, CHCN=1, PNB=1, CNSM=0, SCN=8,
                    // PLSZ=0, mapwh=4, plane base 0x60000.
                    const unsigned int sampleX =
                        static_cast<unsigned int>(exactX) & 0x7FFu;
                    const unsigned int sampleY =
                        static_cast<unsigned int>(exactY) & 0x7FFu;
                    const unsigned int planeX = sampleX / 512u;
                    const unsigned int planeY = sampleY / 512u;
                    const unsigned int inPlaneX = sampleX % 512u;
                    const unsigned int inPlaneY = sampleY % 512u;
                    const unsigned int patternX = inPlaneX / 16u;
                    const unsigned int patternY = inPlaneY / 16u;
                    const unsigned int dotX = inPlaneX % 16u;
                    const unsigned int dotY = inPlaneY % 16u;
                    const unsigned int planeNumber =
                        planeY * 4u + planeX;
                    const unsigned int planeBase =
                        state.planeB[planeNumber & 15u];
                    const unsigned int patternAddr =
                        (planeBase +
                         (patternY * 32u + patternX) * 2u) & 0x7FFFFu;
                    const unsigned int patternName =
                        (static_cast<unsigned int>(liveVram[patternAddr]) << 8) |
                        static_cast<unsigned int>(
                            liveVram[(patternAddr + 1u) & 0x7FFFFu]);
                    const unsigned int flip =
                        (patternName >> 10) & 3u;
                    const unsigned int charNumber =
                        ((patternName & 0x3FFu) << 2) |
                        (8u & 3u) |
                        ((8u & 0x1Cu) << 10);
                    unsigned int sx = dotX;
                    unsigned int sy = dotY;
                    if (flip != 0u) {
                        sy &= 15u;
                        if (flip & 2u) {
                            if ((sy & 8u) == 0u)
                                sy = 7u - sy + 16u;
                            else
                                sy = 15u - sy;
                        } else if (sy & 8u) {
                            sy += 8u;
                        }

                        if (flip & 1u) {
                            if ((sx & 8u) == 0u)
                                sy += 8u;
                            sx &= 7u;
                            sx = 7u - sx;
                        } else if (sx & 8u) {
                            sy += 8u;
                            sx &= 7u;
                        } else {
                            sx &= 7u;
                        }
                    } else {
                        sy &= 15u;
                        if (sy & 8u)
                            sy += 8u;
                        if (sx & 8u)
                            sy += 8u;
                        sx &= 7u;
                    }
                    const unsigned int charAddr =
                        (charNumber * 0x20u + sy * 8u + sx) & 0x7FFFFu;
                    const unsigned int dotColor = liveVram[charAddr];
                    const unsigned int paladdr =
                        (patternName & 0x7000u) >> 4;
                    const unsigned int paletteEntry =
                        paladdr | dotColor;
                    const unsigned int cramAddr =
                        0x80000u + (paletteEntry * 2u);
                    const unsigned int cramOffset =
                        cramAddr - 0x80000u;
                    const unsigned char* liveCram =
                        getVdp2Cram(0);
                    const unsigned int color =
                        (static_cast<unsigned int>(
                            liveCram[cramOffset & 0xFFFu]) << 8) |
                        static_cast<unsigned int>(
                            liveCram[(cramOffset + 1u) & 0xFFFu]);

                    lagi::platform::logging::writef(
                        "[D5RBGSampleB] xy=%u,%u map=%u,%u plane=%u "
                        "paddr=%05X pname=%04X flip=%u char=%u "
                        "caddr=%05X dot=%02X pal=%03X color=%04X\n",
                        x, y, sampleX, sampleY, planeNumber,
                        patternAddr, patternName, flip, charNumber,
                        charAddr, dotColor, paletteEntry, color);
                }
            }

            loggedD5RbgParameterB = true;
        }

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
    } else if (gGameStatus.m0_gameMode == 1 ||
               gGameStatus.m0_gameMode == 3) {
        // The frame that *enters* a native scene began as a movie/module-manager
        // frame and therefore did not acquire the scene producer slot. Release
        // the retained movie immediately, but publish only on a frame that
        // began in a supported native scene mode.
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
                    "[AzelScene] mode=%d heartbeat tasks=%d submissions=%u\n",
                    static_cast<int>(gGameStatus.m0_gameMode),
                    numActiveTask,
                    static_cast<unsigned int>(
                        lagi::azel_bridge::published_submissions().size()));
            }
        }
    } else if (gGameStatus.m0_gameMode != 0) {
        // Other native gameplay modes are not yet presented by Neptune.
        lagi::platform::renderer::movie_clear_frame();
    }

    // Upstream PDS.cpp services the sound interrupt side after the host frame
    // is presented. Keep that split here so the emulated 68000/SCSP command
    // handshake advances with the same frame lifecycle.
    updateSoundInterrupt();

    ++startupFrame;
}

} // namespace lagi::azel
