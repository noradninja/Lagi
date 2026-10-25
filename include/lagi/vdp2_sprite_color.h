#pragma once
#include <cstdint>
namespace lagi {
// Frame-matched VDP2 state. This belongs to the final VDP1/VDP2 compositor,
// not the VDP1 material decoder (PMOD half-transparency is a separate stage).
struct Vdp2SpriteColorState {
    std::uint16_t ccctl=0,spctl=0,craofb=0;
    std::uint16_t priority[4]{},ratio[4]{};
    std::uint16_t colorWindow=0,window[8]{};
    std::uint16_t bgon=0,prir=0,prina=0,prinb=0;
};
// Type-3 indexed VDP1 pixels: retain PRIS/CCRS selectors after CRAM lookup.
// 128 is reserved for a VDP1 half-transparent RGB result; 255 is direct RGB.
constexpr std::uint32_t spriteType3Metadata(std::uint32_t rgba,unsigned code) {
    return (rgba&0xFF000000u) ? (rgba&0xFFFFFFu) |
        ((129u+(((code>>13)&3u)*8u)+((code>>11)&3u))<<24) : 0u;
}
constexpr unsigned spritePriority(const Vdp2SpriteColorState& s,unsigned bank) {
    return (s.priority[(bank&7)/2]>>((bank&1)*8))&7;
}
constexpr unsigned spriteColorRatio(const Vdp2SpriteColorState& s,unsigned bank) {
    return (s.ratio[(bank&7)/2]>>((bank&1)*8))&31;
}
constexpr bool spriteColorCalculationSelected(const Vdp2SpriteColorState& s,
    unsigned priority,bool msb,bool insideColorWindow=true) {
    if (!(s.ccctl&0x40) || !insideColorWindow) return false;
    const unsigned threshold=(s.spctl>>8)&7;
    switch((s.spctl>>12)&3) {
    case 0:return priority<=threshold;
    case 1:return priority==threshold;
    case 2:return priority>=threshold;
    default:return msb;
    }
}
// Type-3 metadata is kept in the completed VDP1 image, not inferred from a
// command's CMDCOLR (LUT texels can select different priority registers).
constexpr unsigned spritePixelPriority(const Vdp2SpriteColorState& s,unsigned code) {
    if(code<128)return 0;
    const unsigned bank=(code>128 && code<255)?(code-129)/8:0;
    return spritePriority(s,bank<3?bank:3);
}
// Existing NBG1 backing/NBG3 glyph draws can be masked against the completed
// VDP1 image without another surface. This is exact when both UI layers are
// above RBG0 and any sprite that beats them is opaque at the VDP2 stage.
// Other states require a top/second-source resolver, not this fast path.
constexpr bool spriteUiPrioritySupported(const Vdp2SpriteColorState& s) {
    if((s.spctl&15)!=3 || !(s.spctl&0x20) || (s.bgon&5) ||
        s.colorWindow || (s.ccctl&0x610))return false;
    unsigned first=8,last=0;
    if(s.bgon&2) {
        first=last=(s.prina>>8)&7;
        if(!first || first<=(s.prir&7))return false;
    }
    if(s.bgon&8) {
        const unsigned p=(s.prinb>>8)&7;
        if(!p || (last && p<=last) || p<=(s.prir&7))return false;
        if(p<first)first=p;
        last=p;
    }
    if(!last)return false;
    for(unsigned code=128;code<256;++code) {
        const unsigned p=spritePixelPriority(s,code);
        if(p>=first && spriteColorCalculationSelected(s,p,code==128 || code==255))
            return false;
    }
    return true;
}
// Resolve register-only decisions once, not at every output pixel. R/G are
// operations with/without an RBG pixel: 0=background, 1=opaque, 2=ratio, 3=add.
// B retains the integer CCRS ratio; A retains the resolved sprite priority.
// Texture storage is RGBA8, point sampled.
constexpr std::uint32_t spriteCompositeLookup(const Vdp2SpriteColorState& s,unsigned code) {
    if(code<128)return 0;
    unsigned p=spritePriority(s,0),r=spriteColorRatio(s,0);
    bool msb=true;
    if(code>128 && code<255) {
        const unsigned selectors=code-129;
        const unsigned pi=selectors/8,ri=selectors%8;
        p=spritePriority(s,pi<3?pi:3);
        r=spriteColorRatio(s,ri<3?ri:3);
        msb=false;
    }
    if(!p)return 0;
    const unsigned op=spriteColorCalculationSelected(s,p,msb)?((s.ccctl&0x100)?3:2):1;
    const unsigned present=(s.prir&7)>p?0:op;
    return present|(op<<8)|(r<<16)|(p<<24);
}
// Exact RGB555 ratio oracle. The GPU compositor must reproduce this rather
// than interpreting CCRS as an ordinary source alpha divided by 31.
constexpr std::uint16_t spriteRatioRgb555(std::uint16_t top,
    std::uint16_t second,unsigned ratio) {
    unsigned out=0x8000;
    ratio&=31;
    for(unsigned shift=0;shift<15;shift+=5)
        out|=(((((top>>shift)&31)*(31-ratio)+
            ((second>>shift)&31)*(ratio+1))>>5)<<shift);
    return static_cast<std::uint16_t>(out);
}
}
