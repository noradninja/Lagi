#pragma once
#include <cstdint>
namespace lagi {
struct SceneNbg0State {
    bool enabled=false;
    bool additive=false;
    // Native normal-background packet; no camera/effect-specific positions.
    float gpu[8][4]{};
};
struct Nbg0ResolveClip {unsigned left=0,top=0,right=351,bottom=223;bool empty=false;};
inline Nbg0ResolveClip sceneNbg0ResolveClip(const SceneNbg0State& s) {
    // Conservative tile-sized clip for one enabled rectangular inside window.
    // Other window combinations retain the full raster and exact shader test.
    if((unsigned(s.gpu[6][0])&15u)!=3u || (unsigned(s.gpu[6][3])&1u))return {};
    int l=int(s.gpu[4][0]),t=int(s.gpu[4][1]),r=int(s.gpu[4][2]),b=int(s.gpu[4][3]);
    if(l>r || t>b || r<0 || b<0 || l>351 || t>223)return {0,0,0,0,true};
    l=l<0?0:l;t=t<0?0:t;r=r>351?351:r;b=b>223?223:b;
    return {unsigned(l)&~31u,unsigned(t)&~31u,
        unsigned(r)|31u,unsigned(b)|31u,false};
}
constexpr bool sceneNbg0Supported(unsigned chctla,unsigned pncn,unsigned ccctl,unsigned plsz) {
    // Tile16, 4bpp, one-word names with native flip bits. Additive and opaque
    // composition use the existing SGX sampling route. Ratio mode is not
    // silently replaced by additive blending.
    return (chctla&0x7f)==1 && (pncn&0xc000)==0x8000 && (plsz&3)!=2 &&
        (!(ccctl&1) || (ccctl&0x100));
}
constexpr unsigned sceneNbg0MapBase(unsigned mpofn,unsigned page,unsigned plsz) {
    const unsigned pages=1u<<((plsz&1)+((plsz>>1)&1));
    return ((((mpofn&7)*64+(page&63))&~(pages-1))*0x800)&0x7ffff;
}
constexpr bool sceneNbg0OverlayBand(unsigned bgon,unsigned prina,unsigned prisa) {
    const unsigned p=prina&7;
    return p>(prisa&7) && (!(bgon&2) || p<((prina>>8)&7));
}
}
