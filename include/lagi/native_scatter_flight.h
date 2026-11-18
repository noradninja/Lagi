#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace lagi {

// BTL_A5.PRG 0605EFEC..0605F090: the fleeing enemy seeds an offset
// vector from its position relative to the battle reference, applies a
// negative Y bias (literal -0x14000), then normalizes to 0x60000.
// The typed native Vita battle-center is a stand-in for the original
// camera/reference service until that service's world transform is ported.
inline std::array<std::int32_t, 3> nativeScatterFlightOffset(
    std::int32_t dx, std::int32_t dy, std::int32_t dz)
{
    const double x = static_cast<double>(dx);
    const double y = static_cast<double>(dy) - 0x14000;
    const double z = static_cast<double>(dz);
    const double length = std::sqrt(x*x + y*y + z*z);
    if (length <= 0.0)
        return {0, 0, 0x60000};
    const double scale = 0x60000 / length;
    return {static_cast<std::int32_t>(x * scale),
            static_cast<std::int32_t>(y * scale),
            static_cast<std::int32_t>(z * scale)};
}

} // namespace lagi
