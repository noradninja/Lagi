#pragma once
#include <cstddef>
#include <cstdint>

namespace lagi {
// Immutable menu NBG2 snapshot; raw Saturn register values, no menu IDs.
struct MenuNbg2State {
    std::uint16_t bgon=0,chctlb=0,pncn=0,plsz=0,mpofn=0,mpab=0,mpcd=0;
    std::uint16_t craofa=0,prina=0,prinb=0,wctlb=0,ccctl=0,ccrnb=0;
    std::uint16_t window[4]{};
    std::int16_t scrollX=0,scrollY=0;
};

inline bool menuNbg2Supported(const MenuNbg2State& state)
{
    // Native menu format: 16x16 4bpp, 1-word/flip names, 1x1-page planes.
    // Do not reinterpret other native layouts as this one.
    return (state.bgon&4) && (state.prinb&7) &&
        (state.chctlb&3)==1 && (state.pncn&0xC000)==0x8000 &&
        (state.plsz&0x30)==0 && (state.wctlb&0xFC)==0 &&
        (state.bgon&0x400)==0 && (state.ccctl&0x300)==0;
}

inline bool menuNbg2VisibleAt(const MenuNbg2State& state,int x,int y)
{
    if (!(state.wctlb&2)) return true;
    const int x0=(state.window[0]>>1)&0x1FF, x1=(state.window[2]>>1)&0x1FF;
    const int y0=state.window[1]&0x1FF, y1=state.window[3]&0x1FF;
    const bool inside=x>=x0 && x<=x1 && y>=y0 && y<=y1;
    return (state.wctlb&1) ? inside : !inside;
}

inline std::size_t menuNbg2PatternAddress(const MenuNbg2State& state,int x,int y)
{
    const unsigned mapX=static_cast<unsigned>(x+int(state.scrollX))&1023;
    const unsigned mapY=static_cast<unsigned>(y+int(state.scrollY))&1023;
    const unsigned plane=(mapY>>9)*2+(mapX>>9);
    const unsigned packed=plane<2 ? state.mpab : state.mpcd;
    const unsigned page=(packed>>((plane&1)*8))&63;
    const unsigned base=(((state.mpofn>>8)&7)*64+page)*0x800;
    return base+(((mapY&511)>>4)*32+((mapX&511)>>4))*2;
}

inline std::uint32_t menuNbg2Blend(const MenuNbg2State& state,
    std::uint32_t top,std::uint32_t second)
{
    if (!top) return second;
    if (!(state.ccctl&4)) return top;
    // VDP2 ratio mode: top:second = (31-ratio):(ratio+1), denominator32.
    // Calculate in RGB555, then expand, matching the original color domain.
    const unsigned ratio=state.ccrnb&31;
    std::uint32_t result=0xFF000000;
    for(unsigned shift=0;shift<24;shift+=8) {
        const unsigned a=(((top>>shift)&255)*31+127)/255;
        const unsigned b=(((second>>shift)&255)*31+127)/255;
        const unsigned color=(a*(31-ratio)+b*(ratio+1))>>5;
        result|=(color*255/31)<<shift;
    }
    return result;
}

inline std::uint32_t menuNbg2Compose(const MenuNbg2State& state,
    std::uint32_t nbg0,std::uint32_t nbg1,std::uint32_t nbg2)
{
    const unsigned p0=state.prina&7,p1=(state.prina>>8)&7,p2=state.prinb&7;
    if (!(state.bgon&1) || !p0) nbg0=0;
    if (!(state.bgon&2) || !p1) nbg1=0;
    const auto backing=nbg0 && (!nbg1 || p0>=p1) ? nbg0 : nbg1;
    if ((nbg0 && p0>=p2) || (nbg1 && p1>=p2)) return backing;
    return menuNbg2Blend(state,nbg2,backing);
}

// One immutable published-frame mixer. Inputs are expanded native RGB555
// palette colors, so channel>>3 recovers the original five bits exactly.
// Hoist register decoding and all ratio arithmetic out of the pixel loop.
struct MenuNbg2Mixer {
    unsigned p0,p1,p2;
    bool blend;
    std::uint8_t channel[32*32]; // Filled completely iff blend can read it.
    explicit MenuNbg2Mixer(const MenuNbg2State& state,bool active=true)
        : p0((state.bgon&1)?state.prina&7:0),
          p1((state.bgon&2)?(state.prina>>8)&7:0),p2(state.prinb&7),
          blend(active && (state.ccctl&4)) {
        if(!blend)return;
        const unsigned ratio=state.ccrnb&31;
        for(unsigned a=0;a<32;++a)for(unsigned b=0;b<32;++b)
            channel[a*32+b]=static_cast<std::uint8_t>(
                (((a*(31-ratio)+b*(ratio+1))>>5)*255)/31);
    }
    std::uint32_t compose(std::uint32_t nbg0,std::uint32_t nbg1,std::uint32_t nbg2) const {
        if(!p0)nbg0=0;
        if(!p1)nbg1=0;
        const auto backing=nbg0 && (!nbg1 || p0>=p1)?nbg0:nbg1;
        if((nbg0 && p0>=p2)||(nbg1 && p1>=p2)||!nbg2)return backing;
        if(!blend)return nbg2;
        const auto r=channel[((nbg2>>3)&31)*32+((backing>>3)&31)];
        const auto g=channel[((nbg2>>11)&31)*32+((backing>>11)&31)];
        const auto b=channel[((nbg2>>19)&31)*32+((backing>>19)&31)];
        return 0xFF000000u|std::uint32_t(r)|(std::uint32_t(g)<<8)|(std::uint32_t(b)<<16);
    }
};
} // namespace lagi
