#include "lagi/lagi_town_tasks.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_runtime.h"
#include "lagi/lagi_town_bootstrap.h"
#include "lagi/platform.h"

#include "town/town.h"
#include "town/ruin/twn_ruin.h"
#include "kernel/fade.h"

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
    if (fadeActive == g_fadeActive)
        return;

    const int remaining = g_fadeControls.m0_fade0.m1E_counter;
    const unsigned int frames =
        static_cast<unsigned int>(remaining > 0 ? remaining : 1);
    if (fadeActive)
        platform::renderer::town_fade_in(frames);
    else
        platform::renderer::town_fade_out(frames);
    g_fadeActive = fadeActive;
}

} // namespace lagi::azel
