#pragma once

#include <cstdint>

namespace lagi {

// The battle target system computes m5A each frame from the native
// high-nibble quadrant mask, selectable state and target order.
// The weak-marker task also retains selection bit 0x200000 between
// updates. A stale selection must not expose an off-quadrant weak point.
inline bool nativeWeakMarkerVisible(std::uint32_t targetFlags,
                                    int selectableIndex,
                                    unsigned quadrant)
{
    if (quadrant >= 4 || selectableIndex < 1)
        return false;
    constexpr std::uint32_t selected = 0x00200000u;
    constexpr std::uint32_t disabled = 0x00100000u;
    constexpr std::uint32_t retired = 0x00040000u;
    return (targetFlags & selected) &&
           !(targetFlags & (disabled | retired)) &&
           (targetFlags & (0x80000000u >> quadrant));
}

} // namespace lagi
