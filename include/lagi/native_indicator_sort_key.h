#pragma once
#include <cstdint>
namespace lagi {
// BTL_A3 060691A6..B2 and all nine target-indicator packet sites:
// MOV.L graphicEngineStatus+4094; SHLL8; SHLL2; SHLL2; SHLR16.
// Keep SH2 unsigned 32-bit wrap, not a widened fixed-point multiply.
constexpr std::uint16_t nativeIndicatorSortKey(std::uint32_t reciprocalFar) {
    return static_cast<std::uint16_t>((reciprocalFar<<12)>>16);
}
}
