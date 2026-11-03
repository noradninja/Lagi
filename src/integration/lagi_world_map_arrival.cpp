#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_arrival.h"
#include "lagi/world_map_display.h"
#include "commonOverlay.h"
#include "dragonData.h"
#include "kernel/animation.h"
#include "kernel/fileBundle.h"
#include "menu_dragonMorph.h"

void setupDragonForTown(s_3dModel*);
void dragonMenuDragonInitSub3(s3DModelAnimData*);

namespace lagi::worldmap {
bool initialize_map_arrival_animation(MapArrivalAnimation& state, s_workArea* owner)
{
    state = {};
    // WORLD root 06054074 loads a dragon only if one is not already resident.
    if (!gDragonState) loadDragon(owner);
    if (!gDragonState || !gDragonState->m0_pDragonModelBundle ||
        !gDragonState->m20_dragonAnimOffsets || !gDragonState->m24_dragonAnimCount)
        return false;
    auto* animation = gDragonState->m0_pDragonModelBundle->getAnimation(
        gDragonState->m20_dragonAnimOffsets[0]);
    if (!animation) return false;
    // Arrival Init 0605536E..9A: release interpolation, select native animation
    // zero from this dragon's own bundle, then reset runtime root deltas.
    setupDragonForTown(&gDragonState->m28_dragon3dModel);
    setupModelAnimation(&gDragonState->m28_dragon3dModel, animation);
    dragonMenuDragonInitSub3(&gDragonState->m78_animData);
    state.ready = true;
    return true;
}

void update_map_arrival_animation(MapArrivalAnimation& state)
{
    if (!state.ready || !gDragonState) return;
    auto& model = gDragonState->m28_dragon3dModel;
    // WORLD 06055606 maintains child +4 at the model's cycle boundary.
    // COMMON 206AC4 has five pointer slots per dragon type. Retail arrival
    // keeps child +1C (the rotation Z/curve selector) at zero.
    const int last = model.m30_pCurrentAnimation ?
        int(s16(model.m30_pCurrentAnimation->m4_numFrames)) - 1 : -1;
    if (s16(model.m16_previousAnimationFrame) >= last) {
        state.curveFrame = s32(u32(state.curveFrame) + 1);
        const auto descriptor = readSaturnEA(gCommonFile->getSaturnPtr(
            0x206AC4 + gDragonState->mC_dragonType * 20));
        if (state.curveFrame >= readSaturnS16(descriptor)) state.curveFrame = 0;
    }
    stepAnimation(&model);
    updateAnimationMatrices(&gDragonState->m78_animData, &model);
}

void draw_map_arrival(const MapArrivalAnimation& state, MapDisplayInput& input,
                      std::int32_t cameraHeight)
{
    // WORLD 060556DA: only the map camera height gates this child. It is not
    // hidden merely because arrival movement has not started yet.
    if (!state.ready || !input.arrivalReady || !gDragonState || cameraHeight > 0x64000)
        return;
    submitModelAndShadowModelToRendering(&gDragonState->m28_dragon3dModel,
        gDragonState->m14_modelIndex, gDragonState->m18_shadowModelIndex,
        &input.arrivalPosition, &input.arrivalRotation, 0);
}
}
