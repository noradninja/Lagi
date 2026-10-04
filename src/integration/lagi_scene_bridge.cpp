#include "lagi/lagi_scene_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_tasks.h"

#include "kernel/moduleManager.h"

namespace lagi::scene_bridge {

void sync_presentation_state()
{
    switch (gGameStatus.m0_gameMode) {
    case 1:
        // Town remains Azel-owned. The town adapter only exposes the camera,
        // player pose and other renderer-facing state produced by that task
        // graph. Additional game modes can be added here as their generic
        // presentation adapters come online.
        lagi::azel::twn_ruin_sync_platform_state();
        break;
    default:
        break;
    }
}

} // namespace lagi::scene_bridge
