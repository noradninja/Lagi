#pragma once
#include <cstddef>
#include <cstdint>
#include "lagi/vdp2_sprite_color.h"

namespace lagi {
// Sparse, deterministic readback of a completed VDP1 A8B8G8R8 surface.
// The high byte is the native selector preserved by the existing renderer;
// no RGB or alpha values are modified by this audit.
struct Vdp1SurfacePriorityAudit {
    unsigned samples=0,empty=0,half=0,indexed=0,direct=0,unknown=0;
    unsigned below=0,equal=0,above=0,nonzeroWithoutPriority=0;
    unsigned regionsBelow[9]{},regionsAbove[9]{};
};
inline Vdp1SurfacePriorityAudit auditVdp1SurfacePriorities(
    const std::uint32_t* pixels,unsigned width,unsigned height,
    unsigned pitch,unsigned spacing,const Vdp2SpriteColorState& state) {
    Vdp1SurfacePriorityAudit a{};
    if(!pixels || !width || !height || pitch<width || !spacing)return a;
    const unsigned rp=state.prir&7u;
    for(unsigned y=spacing/2;y<height;y+=spacing) {
        for(unsigned x=spacing/2;x<width;x+=spacing) {
            ++a.samples;
            const unsigned code=pixels[std::size_t(y)*pitch+x]>>24;
            if(code<128u){++a.empty;continue;}
            if(code==128u)++a.half;
            else if(code==255u)++a.direct;
            else if(code>=129u && code<=160u)++a.indexed;
            else {++a.unknown;continue;}
            const unsigned priority=spritePixelPriority(state,code);
            if(!priority) {++a.nonzeroWithoutPriority;continue;}
            const unsigned region=(y*3u/height)*3u+(x*3u/width);
            if(priority<rp) {++a.below;++a.regionsBelow[region];}
            else if(priority==rp)++a.equal;
            else {++a.above;++a.regionsAbove[region];}
        }
    }
    return a;
}
}
