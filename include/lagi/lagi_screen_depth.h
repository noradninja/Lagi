#pragma once

namespace lagi {
// Identical z/w mapping to Neptune's native left-handed perspective matrix.
// Preserve values outside [0,1] for the GPU clipper; do not clamp to a plane.
inline float native_screen_depth(float viewZ, float nearZ, float farZ)
{
    return (farZ / (farZ - nearZ)) * (1.0f - nearZ / viewZ);
}
}
