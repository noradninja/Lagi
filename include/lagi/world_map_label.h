#pragma once
#include <cstdint>

namespace lagi::worldmap {
// Camera +28/+2C/+30, consumed by the native VDP1 text task. Keep the third
// component raw until its text-task consumer is ported; it is not font scale.
struct MapLabelPlacement {
    std::int32_t x, y, nativeZ;
};
// Include after the Azel prelude for its native vector type.
MapLabelPlacement project_map_label(const sVec3_FP&, std::int16_t halfWidth);

// WORLD 060574E4, after the native transform/divide/multiply projection.
inline MapLabelPlacement bound_map_label(std::int32_t projectedX,
                                         std::int32_t projectedY,
                                         std::int16_t halfWidth)
{
    auto x = std::int32_t(std::uint32_t(projectedX) - std::uint32_t(halfWidth));
    auto y = std::int32_t(std::uint32_t(projectedY) - 3u);
    if (x > 512 || x < -512 || y > 512 || y < -512)
        return {0, -256, 0x333};
    if (x < -160) x = -160;
    const auto right = std::int32_t(160u - (std::uint32_t(halfWidth) << 1));
    if (x > right) x = right;
    if (y > 104) y = 104;
    if (y < -88) y = -88; // MOV #A8 sign-extends; this is not positive 168.
    return {x, y, 0x333};
}
}
