#include "lagi/lagi_town_tasks.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_runtime.h"
#include "lagi/lagi_town_bootstrap.h"
#include "lagi/lagi_direct_boot.h"
#include "lagi/debug_mesh.h"
#include "lagi/platform.h"

#include "town/town.h"
#include "town/townMainLogic.h"
#include "town/townEdge.h"
#include "town/ruin/twn_ruin.h"
#include "kernel/fade.h"
#include "kernel/moduleManager.h"
#include "3dEngine.h"\n#include "VDP2.h"

#include <algorithm>
#include <cmath>

// This file is intentionally a thin adapter.
//
// Runtime/gameplay ownership belongs to upstream Azel in extern/Azel.
// Lagi may create/schedule the upstream task graph and provide platform
// services, but it must not reproduce town script, NPC, Edge, camera,
// animation, or collision behavior here.

// Upstream module-manager callbacks are intentionally reused directly. The
// direct-boot adapter supplies only a minimal initializer so Azel can resume
// from the already-running Ruins state instead of resetting into a new game.
void moduleManager_Update(s_moduleManager* pWorkArea);
void moduleManager_Draw(s_moduleManager* pWorkArea);

namespace lagi::azel {
namespace {

p_workArea g_twnRuinRoot = nullptr;
s_moduleManager* g_directBootModuleManager = nullptr;
sCameraTask* g_fadeCamera = nullptr;
bool g_fadeActive = false;
bool g_reportedPresentation = false;

} // namespace

bool start_twn_ruin_task_pipeline()
{
    if (!town_runtime().initialized || !town_overlay_file())
        return false;

    // Match the gameplay-visible VDP1 projection state that upstream
    // initVDP1() establishes before loadTownPrg(). Neptune owns the Vita
    // renderer, so do not start Azel's desktop VDP1 backend; only restore the
    // screen extents consumed by Azel's culling math, then let Azel compute
    // the actual town projection ratios through initTownProjection().
    graphicEngineStatus.m405C.VDP1_X1 = 0;
    graphicEngineStatus.m405C.VDP1_Y1 = 0;
    graphicEngineStatus.m405C.VDP1_X2 = 352;
    graphicEngineStatus.m405C.VDP1_Y2 = 224;

    reset3dEngine();
    initTownProjection();
    initTownGrid();
    platform::logging::writef(
        "[LagiAdapter] upstream town engine/grid/projection state initialized "
        "wr=%.5f wr2=%.5f\n",
        graphicEngineStatus.m405C.m2C_widthRatio.asS32() / 65536.0f,
        graphicEngineStatus.m405C.m28_widthRatio2.asS32() / 65536.0f);

    // Direct boot bypasses createModuleManager() because its normal Init
    // resets the save/game state and starts a new game. Restore the exact
    // module-manager-visible state that Azel would have established before
    // entering this already-running town. The manager must outlive the overlay:
    // in normal Azel the town is a child of the module manager, never its
    // parent. Keeping that lifetime relationship is essential because
    // terminateTown() finishes the town root and the manager must survive to
    // dispatch the next game status.
    const DirectBootTarget& boot = direct_boot_target();
    if (!boot.resolved) {
        platform::logging::writef(
            "[LagiAdapter] direct-boot target unavailable for module manager\n");
        return false;
    }

    gGameStatus.m0_gameMode = static_cast<s8>(boot.gameMode);
    gGameStatus.m1 = static_cast<s8>(boot.gameModeEntry);
    gGameStatus.m2 = 0;
    gGameStatus.m3_loadingSaveFile = 0;
    gGameStatus.m4_gameStatus = boot.gameStatus;
    gGameStatus.m6_previousGameStatus = 0;
    gGameStatus.m8_nextGameStatus = 0;

    g_directBootModuleManager = new s_moduleManager();
    if (!createRootTask(static_cast<p_workArea>(g_directBootModuleManager))) {
        platform::logging::writef(
            "[LagiAdapter] failed to create upstream module manager root\n");
        return false;
    }
    g_directBootModuleManager->m_UpdateMethod = &moduleManager_Update;
    g_directBootModuleManager->m_DrawMethod = &moduleManager_Draw;
    g_directBootModuleManager->m_DeleteMethod = nullptr;
    g_directBootModuleManager->getTask()->m_taskName =
        s_moduleManager::getTaskName();
    g_directBootModuleManager->state = 0;
    g_directBootModuleManager->m4 = 0;
    g_directBootModuleManager->m6_debugGameStatus =
        static_cast<s16>(gGameStatus.m4_gameStatus);
    g_directBootModuleManager->m8 = nullptr;
    g_directBootModuleManager->mC = 0;
    gModuleManager = g_directBootModuleManager;

    // Match Azel's createLocationTask(): create the town root beneath the
    // persistent module manager, publish townDebugTask2, then let the real
    // TWN_RUIN overlay populate that root with its normal task graph.
    TWN_RUIN_data::makeCurrent();
    auto* root = createSubTaskFromFunction<townDebugTask2Function>(
        g_directBootModuleManager,
        &townDebugTask2Function::Update);
    if (!root) {
        platform::logging::writef(
            "[LagiAdapter] failed to create TWN_RUIN town root\n");
        return false;
    }

    townDebugTask2 = root;
    g_twnRuinRoot = overlayStart_TWN_RUIN(root, 0);
    if (!g_twnRuinRoot) {
        platform::logging::writef(
            "[LagiAdapter] Azel overlayStart_TWN_RUIN failed\n");
        return false;
    }

    g_directBootModuleManager->m8 = g_twnRuinRoot;

    platform::logging::writef(
        "[LagiAdapter] rejoined upstream module manager "
        "status=0x%02X mode=%d entry=0x%02X\n",
        static_cast<unsigned int>(gGameStatus.m4_gameStatus),
        static_cast<int>(gGameStatus.m0_gameMode),
        static_cast<unsigned int>(
            static_cast<std::uint8_t>(gGameStatus.m1)));

    g_fadeCamera = cameraTaskPtr;
    g_fadeActive = false;
    g_reportedPresentation = false;

    // Restore the renderer resource that authentic sEdgeTask::Draw expects.
    // This is presentation-only reconstruction from Azel's already-loaded
    // COMMON3/town resources; gameplay/task ownership remains upstream.
    BasicWingDebugMesh edgeShadow{};
    if (build_edge_shadow_debug_mesh(edgeShadow)) {
        if (!platform::renderer::load_edge_shadow_model(std::move(edgeShadow))) {
            platform::logging::writef(
                "[LagiAdapter] Edge shadow renderer registration failed\n");
        }
    } else {
        platform::logging::writef(
            "[LagiAdapter] Edge shadow reconstruction unavailable\n");
    }

    platform::logging::writef(
        "[LagiAdapter] upstream Azel TWN_RUIN task pipeline started\n");
    return true;
}

bool twn_ruin_task_pipeline_alive()
{
    return g_twnRuinRoot &&
           g_twnRuinRoot->getTask() &&
           !g_twnRuinRoot->getTask()->isFinished();
}

void twn_ruin_frame_begin()
{
    // Azel normally advances this from its Saturn VBlank interrupt. Lagi's
    // native host loop owns VBlank, so provide the one compatibility tick
    // required by script-visible fade state before running the next frame.
    updateFadeInterrupt();
}

void twn_ruin_sync_platform_state()
{
    if (!cameraTaskPtr)
        return;

    if (cameraTaskPtr != g_fadeCamera) {
        g_fadeCamera = cameraTaskPtr;
        g_fadeActive = false;
    }

    const bool fadeActive = cameraTaskPtr->m1_fadeActive != 0;
    if (fadeActive != g_fadeActive) {
        const int remaining = g_fadeControls.m0_fade0.m1E_counter;
        const unsigned int frames =
            static_cast<unsigned int>(remaining > 0 ? remaining : 1);
        if (fadeActive)
            platform::renderer::presentation_fade_in(frames);
        else
            platform::renderer::presentation_fade_out(frames);
        g_fadeActive = fadeActive;
    }

    if (!twnMainLogicTask || !twnMainLogicTask->m14_EdgeTask)
        return;

    constexpr float kInvFixed = 1.0f / 65536.0f;
    constexpr float kTurnsToRadians =
        6.28318530717958647692f / static_cast<float>(0x10000000);
    const auto vec3 = [=](const sVec3_FP& source, float out[3]) {
        out[0] = source[0].asS32() * kInvFixed;
        out[1] = source[1].asS32() * kInvFixed;
        out[2] = source[2].asS32() * kInvFixed;
    };

    sEdgeTask* const edge = twnMainLogicTask->m14_EdgeTask;
    float edgePosition[3]{};
    vec3(edge->mE8.m0_position, edgePosition);
    const unsigned int animation = static_cast<unsigned int>(
        std::max<s32>(0, edge->m2C_currentAnimation.asS32()));
    const unsigned int animationFrame =
        edge->m34_3dModel.m10_currentAnimationFrame;
    platform::renderer::presentation_set_player(
        edgePosition[0], edgePosition[1], edgePosition[2],
        edge->mE8.mC_rotation[1].asS32() * kTurnsToRadians,
        false, 0,
        animation, animationFrame,
        animation, animationFrame, 1.0f);

    float cameraPosition[3]{};
    float rawCameraPosition[3]{};
    float cameraTarget[3]{};
    float cameraUp[3]{};
    vec3(twnMainLogicTask->m38_interpolatedCameraPosition, cameraPosition);
    vec3(twnMainLogicTask->m5C_rawCameraPosition, rawCameraPosition);
    vec3(twnMainLogicTask->m44_cameraTarget, cameraTarget);
    vec3(twnMainLogicTask->m50_upVector, cameraUp);
    platform::renderer::presentation_set_camera(
        cameraPosition,
        rawCameraPosition,
        cameraTarget,
        cameraUp,
        twnMainLogicTask->m68_cameraRotation[1].asS32() * kTurnsToRadians,
        twnMainLogicTask->m68_cameraRotation[0].asS32() * kTurnsToRadians,
        twnMainLogicTask->m24_distance.asS32() * kInvFixed);

    // Azel owns the live town VDP2 maps, CRAM and line-scroll table. Snapshot
    // the exact Saturn-authored state into Neptune's serial presentation
    // handoff. 0x3E000 is the original working line-scroll transfer source
    // used by the town renderer path.
    platform::renderer::presentation_set_vdp2_text(
        getVdp2Vram(0),
        getVdp2Cram(0),
        getVdp2Vram(0x3E000));

    if (!g_reportedPresentation) {
        platform::logging::writef(
            "[LagiAdapter] upstream town presentation ready "
            "edge=(%.5f,%.5f,%.5f) camera=(%.5f,%.5f,%.5f)\n",
            edgePosition[0], edgePosition[1], edgePosition[2],
            cameraPosition[0], cameraPosition[1], cameraPosition[2]);
        g_reportedPresentation = true;
    }

    // Diagnostic only: prove whether Azel's live town VDP2 maps still contain
    // UI/text content before the presentation bridge. Do not publish or alter
    // renderer state here yet.
    static unsigned int vdp2TraceHeartbeat = 0u;
    if ((vdp2TraceHeartbeat++ % 60u) == 0u) {
        const unsigned char* const vram = getVdp2Vram(0);
        unsigned int nbg1Cells = 0u;
        unsigned int textCells = 0u;
        if (vram) {
            constexpr unsigned int kNbg1MapOffset = 0x5800u;
            constexpr unsigned int kNbg1Cells = 32u * 14u;
            constexpr unsigned int kTextMapOffset = 0x6000u;
            constexpr unsigned int kTextCells = 64u * 28u;
            for (unsigned int i = 0; i < kNbg1Cells; ++i) {
                const unsigned int o = kNbg1MapOffset + i * 2u;
                if ((static_cast<unsigned int>(vram[o]) << 8 |
                     static_cast<unsigned int>(vram[o + 1u])) != 0u)
                    ++nbg1Cells;
            }
            for (unsigned int i = 0; i < kTextCells; ++i) {
                const unsigned int o = kTextMapOffset + i * 2u;
                if ((static_cast<unsigned int>(vram[o]) << 8 |
                     static_cast<unsigned int>(vram[o + 1u])) != 0u)
                    ++textCells;
            }
        }
        platform::logging::writef(
            "[PresentationTrace][AzelVDP2] BGON=%04X nbg1Cells=%u textCells=%u\n",
            static_cast<unsigned int>(
                vdp2Controls.m20_registers[0].m20_BGON),
            nbg1Cells,
            textCells);
    }
}

} // namespace lagi::azel
