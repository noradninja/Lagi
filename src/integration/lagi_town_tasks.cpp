#include "lagi/lagi_town_tasks.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_runtime.h"
#include "lagi/lagi_town_bootstrap.h"
#include "lagi/platform.h"

#include "town/town.h"
#include "town/townMainLogic.h"
#include "town/townEdge.h"
#include "town/ruin/twn_ruin.h"
#include "kernel/fade.h"

#include <algorithm>
#include <cmath>

// This file is intentionally a thin adapter.
//
// Runtime/gameplay ownership belongs to upstream Azel in extern/Azel.
// Lagi may create/schedule the upstream task graph and provide platform
// services, but it must not reproduce town script, NPC, Edge, camera,
// animation, or collision behavior here.

namespace lagi::azel {
namespace {

p_workArea g_twnRuinRoot = nullptr;
sCameraTask* g_fadeCamera = nullptr;
bool g_fadeActive = false;
bool g_reportedPresentation = false;

} // namespace

bool start_twn_ruin_task_pipeline()
{
    if (!town_runtime().initialized || !town_overlay_file())
        return false;

    // Instantiate Azel's real TWN_RUIN overlay and let its own
    // overlayStart_TWN_RUIN() construct the script/background/Edge/main/camera
    // task graph. No Lagi-owned duplicate gameplay state is created here.
    TWN_RUIN_data::makeCurrent();

    auto* root = new townDebugTask2Function();
    if (!createRootTask(static_cast<p_workArea>(root)))
        return false;

    root->m_UpdateMethod = &townDebugTask2Function::Update;
    root->m_DrawMethod = nullptr;
    root->m_DeleteMethod = nullptr;
    root->getTask()->m_taskName = townDebugTask2Function::getTaskName();

    g_twnRuinRoot = overlayStart_TWN_RUIN(root, 0);
    if (!g_twnRuinRoot) {
        platform::logging::writef(
            "[LagiAdapter] Azel overlayStart_TWN_RUIN failed\n");
        return false;
    }

    g_fadeCamera = cameraTaskPtr;
    g_fadeActive = false;
    g_reportedPresentation = false;

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
            platform::renderer::town_fade_in(frames);
        else
            platform::renderer::town_fade_out(frames);
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
    platform::renderer::town_present_edge(
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
    platform::renderer::town_present_camera(
        cameraPosition,
        rawCameraPosition,
        cameraTarget,
        cameraUp,
        twnMainLogicTask->m68_cameraRotation[1].asS32() * kTurnsToRadians,
        twnMainLogicTask->m68_cameraRotation[0].asS32() * kTurnsToRadians,
        twnMainLogicTask->m24_distance.asS32() * kInvFixed);

    if (!g_reportedPresentation) {
        platform::logging::writef(
            "[LagiAdapter] upstream town presentation ready "
            "edge=(%.5f,%.5f,%.5f) camera=(%.5f,%.5f,%.5f)\n",
            edgePosition[0], edgePosition[1], edgePosition[2],
            cameraPosition[0], cameraPosition[1], cameraPosition[2]);
        g_reportedPresentation = true;
    }
}

} // namespace lagi::azel
