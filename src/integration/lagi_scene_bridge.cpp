#include "lagi/lagi_scene_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_tasks.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/platform.h"

#include "kernel/moduleManager.h"
#include "field.h"
#include "field/fieldCamera.h"
#include "field/fieldDragon.h"

namespace lagi::scene_bridge {

namespace {

void sync_field_presentation_state()
{
    if (!fieldTaskPtr || !fieldTaskPtr->m8_pSubFieldData)
        return;

    s_FieldSubTaskWorkArea* const field = fieldTaskPtr->m8_pSubFieldData;
    if (!field->m338_pDragonTask || !field->m334)
        return;

    // Phase 2 presentation boundary:
    // Azel's field renderer submits VDP1 geometry with the field camera
    // already folded into pCurrentMatrix. Publish those native submissions
    // through Neptune with an identity host camera for the first visible
    // frame. Phase 4 will classify individual field submission spaces and
    // move world-space paths onto an explicit camera adapter where needed.
    static const float cameraPosition[3] = {0.0f, 0.0f, 0.0f};
    static const float cameraTarget[3] = {0.0f, 0.0f, 1.0f};
    static const float cameraUp[3] = {0.0f, 1.0f, 0.0f};

    lagi::platform::renderer::presentation_set_scene_mode(3u);
    lagi::platform::renderer::presentation_set_camera(
        cameraPosition,
        cameraPosition,
        cameraTarget,
        cameraUp,
        0.0f,
        0.0f,
        0.0f);

    static unsigned int heartbeat = 0u;
    if ((heartbeat++ % 60u) == 0u) {
        const s_dragonTaskWorkArea* const dragon = field->m338_pDragonTask;
        lagi::platform::logging::writef(
            "[LagiFieldAdapter] field=%d sub=%d submissions=%u "
            "dragon=(%08X,%08X,%08X) angle=(%08X,%08X,%08X)\n",
            static_cast<int>(fieldTaskPtr->m2C_currentFieldIndex),
            static_cast<int>(fieldTaskPtr->m2E_currentSubFieldIndex),
            static_cast<unsigned int>(
                lagi::azel_bridge::submission_count()),
            static_cast<unsigned int>(dragon->m8_pos[0].asS32()),
            static_cast<unsigned int>(dragon->m8_pos[1].asS32()),
            static_cast<unsigned int>(dragon->m8_pos[2].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[0].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[1].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[2].asS32()));
    }
}

} // namespace

void sync_presentation_state()
{
    switch (gGameStatus.m0_gameMode) {
    case 1:
        // Town remains Azel-owned. The adapter exposes only renderer-facing
        // state produced by Azel's native town task graph.
        lagi::azel::twn_ruin_sync_platform_state();
        break;
    case 3:
        // Field remains Azel-owned. Phase 2 only opens the generic
        // presentation boundary for its existing model submissions.
        sync_field_presentation_state();
        break;
    default:
        break;
    }
}

} // namespace lagi::scene_bridge
