#pragma once
#include <cstddef>
#include <cstdint>
#include "lagi/vdp2_sprite_color.h"

namespace lagi {
// A CPU-side summary of the native VDP1 pixel selector, not a mesh/entity
// priority override. Bit 0: RGB/half-result (selector 0). Bits 1..4:
// type-3 palette priority selectors 0..3. Transparent texels contribute none.
constexpr std::uint8_t vdp1PrioritySelectorBit(std::uint8_t alpha) {
    if (alpha < 128u) return 0u;
    if (alpha == 128u || alpha == 255u) return 1u;
    if (alpha >= 129u && alpha <= 160u)
        return static_cast<std::uint8_t>(2u << ((alpha-129u)/8u));
    return 0u; // Unknown metadata must never silently become priority 4.
}
inline std::uint8_t vdp1PrioritySelectorCoverage(
    const std::uint32_t* rgba,std::size_t count) {
    std::uint8_t coverage=0u;
    for(std::size_t i=0;i<count;++i)
        coverage|=vdp1PrioritySelectorBit(
            static_cast<std::uint8_t>(rgba[i]>>24));
    return coverage;
}
struct Vdp1PriorityCoverage { bool below=false,above=false; };
// Equal priorities favor the VDP1 image over RBG0.
constexpr Vdp1PriorityCoverage vdp1PriorityCoverage(
    unsigned selectorMask,const Vdp2SpriteColorState& state,unsigned rbgPriority) {
    Vdp1PriorityCoverage result{};
    for(unsigned selector=0;selector<5;++selector) {
        if(!(selectorMask&(1u<<selector)))continue;
        const unsigned bank=selector?selector-1u:0u;
        const unsigned priority=spritePriority(state,bank);
        if(priority<rbgPriority)result.below=true;
        else if(priority>=rbgPriority)result.above=true;
    }
    return result;
}
}
