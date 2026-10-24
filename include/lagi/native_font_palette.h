#pragma once
#include <cassert>
#include <cstdint>

// Owned by Azel VDP2.cpp. Script/result text colors are selectors, not banks.
extern std::uint32_t printVdp2StringTable[10];
namespace lagi {
inline std::uint16_t nativeFontPalette(unsigned selector)
{
    assert(selector<10);
    return static_cast<std::uint16_t>(printVdp2StringTable[selector]<<12);
}
}
