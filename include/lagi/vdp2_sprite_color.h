#pragma once
#include <cstdint>
namespace lagi {
// Frame-matched VDP2 state. This belongs to the final VDP1/VDP2 compositor,
// not the VDP1 material decoder (PMOD half-transparency is a separate stage).
struct Vdp2SpriteColorState {
    std::uint16_t ccctl=0,spctl=0,craofb=0;
    std::uint16_t priority[4]{},ratio[4]{};
    std::uint16_t colorWindow=0,window[8]{};
    std::uint16_t sfsel=0,sfcode=0; // VDP2 0x24/0x26, per-dot special function selection
    std::uint16_t pncn1=0; // NBG1 one-word pattern auxiliary special-color bit
    std::uint16_t nbg1Ratio=0; // CCRNA bits 8-12: NBG1 ratio
    std::uint16_t bgon=0,prir=0,prina=0,prinb=0;
    std::uint16_t rbgRatio=0; // Native CCRR; sprite CCRS is a different selector.
    std::uint16_t specialPriority=0,specialColor=0;
};
// SFCCMD (0x1800EE): two bits per VDP2 background layer.
// 0=NBG0/RBG1, 1=NBG1/EXBG, 2=NBG2, 3=NBG3, 4=RBG0.
// A nonzero NBG1 mode does not change RBG0's color-calculation mode.
constexpr unsigned specialColorMode(const Vdp2SpriteColorState& s,unsigned layer) {
    return layer<5u ? ((s.specialColor>>(layer*2u))&3u) : 0u;
}
// Preserve a per-dot color-calculation selector in the unused intermediate
// atlas alpha byte; this is metadata, never an opacity fraction.
constexpr std::uint32_t nbg1DotMaterial(std::uint32_t abgr,bool selected) {
    return selected && (abgr&0xff000000u)
        ? (abgr&0x00ffffffu)|0x80000000u : abgr;
}
// 1-word NBG1 pattern names with PNCN1=PNB1/CNSM0 obtain their
// special-color bit from auxiliary PNCN1 bit 8 (SCC), not from the
// palette number or character index in the tile map.
constexpr bool nbg1AuxiliarySpecialColor(const Vdp2SpriteColorState& s) {
    return (s.pncn1&0x0100u)!=0u;
}
// Sega VDP2: SFSEL chooses SFCODE A/B for each scroll layer.
// Each SFCODE bit covers TWO adjacent values of dotColor's low nibble.
constexpr bool specialFunctionDotMatch(const Vdp2SpriteColorState& s,
    unsigned layer,unsigned dotColor) {
    if(layer>=5u)return false;
    const unsigned mask=((s.sfsel>>layer)&1u)
        ? ((s.sfcode>>8)&0xffu) : (s.sfcode&0xffu);
    return (mask & (1u<<((dotColor&15u)>>1u)))!=0u;
}
// Normal and special scroll-screen color calculation selection. The special
// mode applies only when this screen is the winning/top priority source.
// patternSpecial is the pattern-name SCC bit (or PNCN auxiliary SCC bit
// for one-word names), and cramMsb is the selected CRAM color's MSB.
constexpr bool scrollSpecialColorCalculationSelected(
    const Vdp2SpriteColorState& s,unsigned layer,bool topImage,
    bool patternSpecial,unsigned dotColor,bool cramMsb) {
    if(layer>=5u || !topImage || !(s.ccctl&(1u<<layer)))return false;
    switch(specialColorMode(s,layer)) {
    case 0:return true; // Entire enabled screen
    case 1:return patternSpecial; // Character
    case 2:return patternSpecial && specialFunctionDotMatch(s,layer,dotColor);
    default:return cramMsb; // CRAM color's MSB
    }
}
constexpr unsigned rbgColorRatio(const Vdp2SpriteColorState& s) {return s.rbgRatio&31u;}
// Type-3 indexed VDP1 pixels: retain PRIS/CCRS selectors after CRAM lookup.
// 128 is reserved for a VDP1 half-transparent RGB result; 255 is direct RGB.
constexpr std::uint32_t spriteType3Metadata(std::uint32_t rgba,unsigned code) {
    return (rgba&0xFF000000u) ? (rgba&0xFFFFFFu) |
        ((129u+(((code>>13)&3u)*8u)+((code>>11)&3u))<<24) : 0u;
}
// Type 0 has two priority bits and THREE ratio bits. Keep the same byte
// namespace as type 3; the frame's SPCTL disambiguates the ratio range.
constexpr std::uint32_t spriteType0Metadata(std::uint32_t rgba,unsigned code) {
    return (rgba&0xFF000000u) ? (rgba&0xFFFFFFu) |
        ((129u+(((code>>14)&3u)*8u)+((code>>11)&7u))<<24) : 0u;
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
// Register-defined three-source route: opaque NBG0, VDP1, RBG0 color
// calculation, optional NBG3 above both background layers. VDP1/UI ties are
// handled by the existing completed-image UI mask. No
// special-function, destination-alpha, extended calculation or color window.
// This deliberately describes a supported state, not a game-mode exception.
constexpr bool rbgLayerCompositeSupported(const Vdp2SpriteColorState& s) {
    if(((s.spctl&15)!=0 && (s.spctl&15)!=3) || !(s.spctl&0x20) || (s.spctl&0x10) ||
        !(s.ccctl&0x10) || (s.ccctl&~0x152u) ||
        s.colorWindow || (s.bgon&0x24) || (s.specialPriority&0x303) ||
        (s.specialColor&0x303))return false;
    // Preserve Saturn NBG1 additive color calculation and its priority.
    // Other NBG1 modes need a general VDP2 top/second-source resolver.
    if(s.bgon&2) {
        if((s.ccctl&0x102u)!=0x102u)return false;
        const unsigned nbg1=(s.prina>>8)&7u;
        if(!nbg1 || nbg1<=(s.prir&7u) ||
            ((s.bgon&1) && nbg1<=(s.prina&7u)))return false;
        if((s.bgon&8) && ((s.prinb>>8)&7u)<=nbg1)return false;
        for(unsigned code=128;code<256;++code) {
            const unsigned priority=spritePixelPriority(s,code);
            if(priority>=nbg1 &&
                spriteColorCalculationSelected(s,priority,code==128 || code==255))
                return false;
        }
    } else if(s.ccctl&2u)return false;
    if(s.bgon&8) {
        const unsigned ui=(s.prinb>>8)&7;
        if((s.specialPriority&0xc0) || !ui || ui<=(s.prir&7) ||
            ((s.bgon&1) && ui<=(s.prina&7)))return false;
        for(unsigned code=128;code<256;++code) {
            const unsigned p=spritePixelPriority(s,code);
            if(p>=ui && spriteColorCalculationSelected(s,p,code==128 || code==255))return false;
        }
    }
    return true;
}
// Existing NBG1 backing/NBG3 glyph draws can be masked against the completed
// VDP1 image without another surface. This is exact when both UI layers are
// above RBG0 and any sprite that beats them is opaque at the VDP2 stage.
// Other states require a top/second-source resolver, not this fast path.
constexpr bool spriteUiPrioritySupported(const Vdp2SpriteColorState& s) {
    if(rbgLayerCompositeSupported(s))return (s.bgon&0xAu)!=0;
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
        r=spriteColorRatio(s,(s.spctl&15)==0?ri:(ri<3?ri:3));
        msb=false;
    }
    if(!p)return 0;
    const unsigned op=spriteColorCalculationSelected(s,p,msb)?((s.ccctl&0x100)?3:2):1;
    const unsigned present=(s.prir&7)>p?0:op;
    return present|(op<<8)|(r<<16)|(p<<24);
}
// Saturn RGB555 additive color calculation saturates each channel
// independently. The top image determines whether this calculation runs;
// a lower-priority VDP1 color supplies the second image unmodified.
constexpr std::uint16_t vdp2AddRgb555(std::uint16_t top,std::uint16_t second) {
    unsigned out=0x8000u;
    for(unsigned shift=0;shift<15;shift+=5) {
        const unsigned sum=((top>>shift)&31u)+((second>>shift)&31u);
        out|=(sum>31u?31u:sum)<<shift;
    }
    return static_cast<std::uint16_t>(out);
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
