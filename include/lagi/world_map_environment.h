#pragma once
#include <cstdint>

namespace lagi::worldmap {
struct MapNormalBackgroundOrigin {
    std::uint32_t horizontal;
    std::uint32_t vertical;      // environment +8, before the consumer branch
    std::uint32_t scrollY;       // value passed to MAIN 06028986
};

// WORLD 06056E58..84 and 06056E94..06056EF8. The consumer compares the
// signed, wrapping sum against 196; it is not a conventional clamp to 236.
// Inputs remain native 16.16 yaw, signed projected ground, and environment +40.
inline MapNormalBackgroundOrigin map_normal_background_origin(
    std::int32_t yaw, std::int32_t ground, std::int32_t verticalOffset)
{
    // Twelve SHAR instructions, then SHLL8/SHLL2/NEG. Spell out unsigned
    // shifts/subtractions so the original low-32-bit wrap is well-defined.
    const auto shiftedYaw = (static_cast<std::uint32_t>(yaw) >> 12) |
        (yaw < 0 ? 0xFFF00000u : 0u);
    const auto horizon = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(ground) +
        static_cast<std::uint32_t>(verticalOffset));
    return {
        0u - (shiftedYaw << 10),
        (256u - static_cast<std::uint32_t>(ground)) << 16,
        horizon >= 196
            ? 0u - (static_cast<std::uint32_t>(horizon) << 16)
            : 0xFF140000u,
    };
}
}
