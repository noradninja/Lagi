#pragma once
#include <array>
#include <cstdint>

namespace lagi {
using NativeRadarQuad = std::array<std::array<std::int16_t, 2>, 4>;
// Original FLD_A3 060726DC. Flags 1/2 select size; 4/8 taper the
// upper/lower edge for height. Other shapes are the native single-pixel quad.
inline NativeRadarQuad native_radar_marker_quad(std::int16_t x, std::int16_t y, unsigned flags)
{
    const auto cell = [](int a, int b) { return std::array<std::int16_t, 2>{static_cast<std::int16_t>(a), static_cast<std::int16_t>(b)}; };
    if ((flags & 3) != 1 && (flags & 3) != 2)
        return {cell(x,y), cell(x+1,y), cell(x+1,y-1), cell(x,y-1)};
    const int r = (flags & 3) == 1 ? 1 : 2;
    NativeRadarQuad q = {cell(x-r,y+r), cell(x+r,y+r), cell(x+r,y-r), cell(x-r,y-r)};
    if (flags & 4) { q[0][0] += r; q[1][0] -= r; }
    else if (flags & 8) { q[3][0] += r; q[2][0] -= r; }
    return q;
}
}
