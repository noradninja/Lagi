#pragma once
#include <cstdint>
namespace lagi {
// PMOD modes 3 and 7: half-transparency, with/without Gouraud.
// Mesh (bit 8) is independent; never infer blending from an effect identity.
constexpr bool vdp1_half_transparent(unsigned pmod) { return (pmod & 3u)==3u; }
constexpr std::uint32_t vdp1_blend_texel(unsigned pmod,std::uint32_t rgba)
{
    return vdp1_half_transparent(pmod) && (rgba & 0xFF000000u)
        ? (rgba & 0x00FFFFFFu) | 0x80000000u : rgba;
}
}
