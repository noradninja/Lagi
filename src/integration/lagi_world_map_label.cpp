#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_label.h"
#include "3dEngine.h"

namespace lagi::worldmap {
MapLabelPlacement project_map_label(const sVec3_FP& position, s16 halfWidth)
{
    // WORLD 060574FC: include the current native camera translation, not just
    // rotation. 06057506 divides 1.0 by view Z using Azel's divider semantics.
    sVec3_FP view;
    transformAndAddVecByCurrentMatrix(&position, &view);
    const auto reciprocal = FP_Div(0x10000, view[2]);
    const auto& projection = graphicEngineStatus.m405C;
    const auto x = MTH_Mul_5_6(projection.m18_widthScale, view[0], reciprocal);
    const auto y = MTH_Mul_5_6(projection.m1C_heightScale, view[1], reciprocal);
    return bound_map_label(x.asS32(), y.asS32(), halfWidth);
}
}
