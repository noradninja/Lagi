#include "../include/lagi/vdp2_sprite_color.h"
#include <cassert>
#include <cstdio>
// Independent transcription of the previous fragment program's decisions.
static unsigned reference(const lagi::Vdp2SpriteColorState& s,unsigned code,bool background) {
    if(code<128)return 0;
    unsigned p=lagi::spritePriority(s,0),r=lagi::spriteColorRatio(s,0),msb=1;
    if(code>128 && code<255) {
        unsigned pi=(code-129)/8,ri=(code-129)%8;
        p=lagi::spritePriority(s,pi<1?0:pi<2?1:pi<3?2:3);
        r=lagi::spriteColorRatio(s,ri<1?0:ri<2?1:ri<3?2:3);
        msb=0;
    }
    if(!p || (background && (s.prir&7)>p))return 0;
    unsigned condition=(s.spctl>>12)&3,threshold=(s.spctl>>8)&7;
    bool selected=condition==0?p<=threshold:condition==1?p==threshold:condition==2?p>=threshold:msb;
    return (selected?((s.ccctl&0x100)?3:2):1)|(r<<8);
}
int main() {
    lagi::Vdp2SpriteColorState s{};
    // Rotate every priority/ratio value through every bank, threshold,
    // condition, background priority and additive/ratio mode, all alpha bytes.
    for(unsigned p=0;p<8;++p)for(unsigned r=0;r<32;++r)
    for(unsigned threshold=0;threshold<8;++threshold)
    for(unsigned condition=0;condition<4;++condition)
    for(unsigned bg=0;bg<8;++bg)for(unsigned add=0;add<2;++add) {
        s.ccctl=0x40|(add<<8);s.spctl=(condition<<12)|(threshold<<8);s.prir=bg;
        s.priority[0]=p|(((p+1)&7)<<8);s.priority[1]=((p+2)&7)|(((p+3)&7)<<8);
        s.ratio[0]=r|(((r+7)&31)<<8);s.ratio[1]=((r+13)&31)|(((r+23)&31)<<8);
        for(unsigned code=0;code<256;++code) {
            auto entry=lagi::spriteCompositeLookup(s,code);
            for(unsigned present=0;present<2;++present) {
                unsigned op=(entry>>(present?0:8))&255;
                unsigned expected=reference(s,code,present!=0);
                assert(op==(expected&255));
                if(op>=2)assert(((entry>>16)&255)==(expected>>8));
            }
        }
    }
    std::puts("Sprite compositor lookup matches prior shader decisions");
}
