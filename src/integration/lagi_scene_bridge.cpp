#include "lagi/lagi_scene_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_tasks.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/platform.h"

#include "kernel/moduleManager.h"
#include "field.h"
#include "field/fieldCamera.h"
#include "field/fieldDragon.h"
#include "field/fieldVisibilityGrid.h"

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
    const float nearPlane =
        static_cast<float>(
            static_cast<s32>(graphicEngineStatus.m405C.m10_nearClipDistance)) /
        65536.0f;
    const float farPlane =
        static_cast<float>(
            static_cast<s32>(graphicEngineStatus.m405C.m14_farClipDistance)) /
        65536.0f;
    lagi::platform::renderer::presentation_set_clip_planes(
        nearPlane, farPlane);

    std::int32_t nativeView[12]{};
    if (lagi::azel_bridge::native_scene_view_matrix(nativeView))
        lagi::platform::renderer::presentation_set_native_view_matrix(
            nativeView);

    lagi::platform::renderer::presentation_set_camera(
        cameraPosition,
        cameraPosition,
        cameraTarget,
        cameraUp,
        0.0f,
        0.0f,
        0.0f);

    const s_dragonTaskWorkArea* const dragon = field->m338_pDragonTask;
    const s_visibilityGridWorkArea* const grid =
        field->m348_pFieldCameraTask1;

    unsigned int activeCells = 0u;
    int gridWidth = 0;
    int gridHeight = 0;
    if (grid && grid->m30) {
        gridWidth = grid->m30->m10_gridSize[0];
        gridHeight = grid->m30->m10_gridSize[1];
        const int cellCount = gridWidth * gridHeight;
        if (grid->m3C_cellRenderingTasks) {
            for (int i = 0; i < cellCount; ++i) {
                s_visdibilityCellTask* const cell =
                    grid->m3C_cellRenderingTasks[i];
                if (cell && cell->getTask() &&
                    !cell->getTask()->isPaused())
                    ++activeCells;
            }
        }
    }

    const int cameraCellX =
        grid ? grid->m18_cameraGridLocation[0] : -999;
    const int cameraCellY =
        grid ? grid->m18_cameraGridLocation[1] : -999;
    static int previousCameraCellX = -1000;
    static int previousCameraCellY = -1000;
    static unsigned int previousActiveCells = 0xFFFFFFFFu;
    if (cameraCellX != previousCameraCellX ||
        cameraCellY != previousCameraCellY ||
        activeCells != previousActiveCells) {
        lagi::platform::logging::writef(
            "[FieldCellTransition] cameraCell=(%d,%d) activeCells=%u "
            "gridSeen=%u gridVisible=%u submissions=%u\n",
            cameraCellX,
            cameraCellY,
            activeCells,
            grid ? static_cast<unsigned int>(grid->m12E0) : 0u,
            grid ? static_cast<unsigned int>(grid->m12E2) : 0u,
            static_cast<unsigned int>(
                lagi::azel_bridge::submission_count()));
        previousCameraCellX = cameraCellX;
        previousCameraCellY = cameraCellY;
        previousActiveCells = activeCells;
    }

    static unsigned int heartbeat = 0u;
    if ((heartbeat++ % 60u) == 0u) {
        lagi::platform::logging::writef(
            "[LagiFieldAdapter] field=%d sub=%d submissions=%u "
            "dragon=(%08X,%08X,%08X) angle=(%08X,%08X,%08X) "
            "grid=%dx%d cameraCell=(%d,%d) activeCells=%u "
            "gridSeen=%u gridVisible=%u renderMode=%u "
            "clip=(%.4f,%.4f)\n",
            static_cast<int>(fieldTaskPtr->m2C_currentFieldIndex),
            static_cast<int>(fieldTaskPtr->m2E_currentSubFieldIndex),
            static_cast<unsigned int>(
                lagi::azel_bridge::submission_count()),
            static_cast<unsigned int>(dragon->m8_pos[0].asS32()),
            static_cast<unsigned int>(dragon->m8_pos[1].asS32()),
            static_cast<unsigned int>(dragon->m8_pos[2].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[0].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[1].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[2].asS32()),
            gridWidth,
            gridHeight,
            cameraCellX,
            cameraCellY,
            activeCells,
            grid ? static_cast<unsigned int>(grid->m12E0) : 0u,
            grid ? static_cast<unsigned int>(grid->m12E2) : 0u,
            grid ? static_cast<unsigned int>(grid->m12F2_renderMode) : 0u,
            nearPlane,
            farPlane);
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
