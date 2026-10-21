#pragma once
#include <cstdint>
namespace lagi {
struct SceneNbg0State {
    bool enabled=false;
    bool additive=false;
    // Native normal-background packet; no camera/effect-specific positions.
    float gpu[8][4]{};
};
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
