#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace lagi {

inline constexpr unsigned kSkyAmbientBlendNumerator = 1u;
inline constexpr unsigned kSkyAmbientBlendDenominator = 4u;

inline std::int64_t sky_ambient_rounded_divide(
    std::int64_t value, std::int64_t divisor) {
    return value >= 0 ? (value + divisor / 2) / divisor
                      : -((-value + divisor / 2) / divisor);
}

// Add only the sky's chroma to the existing native ambient. The channel sum
// remains unchanged unless an extreme input has to be clamped, so this cannot
// brighten or darken the authored depth ambient. A neutral sky is a no-op.
inline void blend_sky_chroma_preserve_brightness(
    int ambient[3], const std::uint8_t sky[3]) {
    const std::int64_t originalSum = std::int64_t(ambient[0]) + ambient[1] + ambient[2];
    const std::int64_t skySum = std::int64_t(sky[0]) + sky[1] + sky[2];
    if (originalSum == 0 || skySum == 0)
        return;

    int mixed[3]{};
    const std::int64_t divisor =
        3 * skySum * kSkyAmbientBlendDenominator;
    for (unsigned channel = 0; channel < 3; ++channel) {
        const std::int64_t centeredSky =
            3 * std::int64_t(sky[channel]) - skySum;
        const std::int64_t chromaDelta = sky_ambient_rounded_divide(
            originalSum * centeredSky * kSkyAmbientBlendNumerator,
            divisor);
        mixed[channel] = static_cast<int>(std::clamp(
            std::int64_t(ambient[channel]) + chromaDelta,
            std::int64_t(std::numeric_limits<int>::min()),
            std::int64_t(std::numeric_limits<int>::max())));
    }

    // Rounding the three zero-sum chroma terms can leave a one- or two-unit
    // residual. Remove it before the renderer's existing RGB555 clamp.
    const std::int64_t mixedSum =
        std::int64_t(mixed[0]) + mixed[1] + mixed[2];
    mixed[2] = static_cast<int>(std::clamp(
        std::int64_t(mixed[2]) + originalSum - mixedSum,
        std::int64_t(std::numeric_limits<int>::min()),
        std::int64_t(std::numeric_limits<int>::max())));
    for (unsigned channel = 0; channel < 3; ++channel)
        ambient[channel] = mixed[channel];
}

} // namespace lagi
