#pragma once

namespace lagi {
// Azel's authored 352x224 raster represents a 4:3 display. The wider Vita
// camera retains this X scale, revealing more world at the sides.
constexpr float saturnDisplayXScale(unsigned width, unsigned height) {
    return (4.0f / 3.0f) / (static_cast<float>(width) / height);
}
constexpr float rbgProjectionInverseXScale(bool frontend, unsigned width, unsigned height) {
    // Title/name-entry screens have no widened 3D camera to match.
    return frontend ? 1.0f : 1.0f / saturnDisplayXScale(width, height);
}
} // namespace lagi
