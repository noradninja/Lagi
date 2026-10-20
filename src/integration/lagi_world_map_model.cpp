#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_model.h"
#include "3dEngine.h"
#include "3dModels.h"
#include "kernel/fileBundle.h"
#include "kernel/animation.h"
#include "mainMenuDebugTasks.h"

namespace lagi::worldmap {
bool initialize_map_model(MapModel& state, s_workAreaCopy* owner,
                          sSaturnPtr lightTable)
{
    if (!owner || !owner->m0_fileBundle || lightTable.isNull()) return false;
    auto* bundle = owner->m0_fileBundle;
    auto* hierarchy = bundle->getModelHierarchy(0x1EC);
    if (!hierarchy) return false;

    // WORLD 06054510..92: initial pitch/yaw lighting descriptor and four RGB
    // triplets at 06058774. The bytes remain game-owned, read at runtime.
    state.position.zeroize();
    state.yaw = 0;
    state.scale = 0;
    state.poseControl = 0;
    state.transitionRate = 0x2000;
    state.flags = 0;
    state.lightAngle = static_cast<s32>(0xFF1C71C8u);
    for (unsigned i = 0; i < state.lightColors.size(); ++i) {
        state.lightColors[i] = readSaturnU8(lightTable + i * 3) |
            (u32(readSaturnU8(lightTable + i * 3 + 1)) << 8) |
            (u32(readSaturnU8(lightTable + i * 3 + 2)) << 16);
    }

    // 060545AA..D8: hierarchy 01EC, no animation, default pose at bundle
    // slot 05C8, color animation at 05D0, then stepAnimation once. Resource
    // offsets are bundle table slots, not addresses to native pointer-cast.
    auto* pose = bundle->getStaticPose(0x5C8, hierarchy->countNumberOfBones());
    if (!pose) return false;
    if (!init3DModelRawData(owner, &state.model, 0, bundle, 0x1EC,
            nullptr, pose, bundle->getRawFileAtOffset(0x5D0), nullptr))
        return false;
    stepAnimation(&state.model);
    return true;
}

void draw_map_model(MapModel& state)
{
    // WORLD 060551AC..204: bit 0x20 is the native visibility gate. Rotate
    // around Y, scale each matrix column, copy +82 to model +14, then call
    // Azel's own model draw function. No proxy terrain or guessed materials.
    if (!(state.flags & 0x20)) return;
    pushCurrentMatrix();
    translateCurrentMatrix(state.position);
    rotateCurrentMatrixY(state.yaw);
    scaleCurrentMatrixRow0(state.scale.asS32());
    scaleCurrentMatrixRow1(state.scale.asS32());
    scaleCurrentMatrixRow2(state.scale.asS32());
    state.model.m14 = state.poseControl;
    state.model.m18_drawFunction(&state.model);
    popMatrix();
}
}
