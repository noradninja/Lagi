#include "lagi/lagi_town_tasks.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_runtime.h"
#include "lagi/platform.h"

#include "town/town.h"
#include "town/ruin/twn_ruin.h"

// This file is intentionally a thin adapter.
//
// Runtime/gameplay ownership belongs to upstream Azel in extern/Azel.
// Lagi may create/schedule the upstream task graph and provide platform
// services, but it must not reproduce town script, NPC, Edge, camera,
// animation, or collision behavior here.

namespace lagi::azel {
namespace {

p_workArea g_twnRuinRoot = nullptr;

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

} // namespace lagi::azel
