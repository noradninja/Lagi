#include "../include/lagi/vdp2_sprite_color.h"
#include <cassert>
#include <cstdio>
static unsigned priority(const lagi::Vdp2SpriteColorState& s,unsigned code) {
    if(code<128)return 0;
    unsigned bank=0;
    if(code!=128 && code!=255)bank=(code-129)/8;
    if(bank>3)bank=3;
    return (s.priority[bank/2]>>((bank%2)*8))&7;
}
int main() {
    lagi::Vdp2SpriteColorState s;
    s.ccctl=0x42;s.spctl=0x1423;s.bgon=0x1A;s.prir=3;
    s.prina=0x600;s.prinb=0x700;
    s.priority[0]=0x405;s.priority[1]=0x507;
    assert(lagi::spriteUiPrioritySupported(s));
    // One LUT command may contain both a weak-marker bank and an above-panel
    // bank. Neither command sort keys nor camera angles decide this boundary.
    assert(lagi::spritePixelPriority(s,129)==5);
    assert(lagi::spritePixelPriority(s,145)==7);
    assert((lagi::spriteCompositeLookup(s,129)>>24)<6);
    assert((lagi::spriteCompositeLookup(s,145)>>24)>=6);
    for(unsigned a=0;a<8;++a)for(unsigned b=0;b<8;++b)
    for(unsigned c=0;c<8;++c)for(unsigned d=0;d<8;++d) {
        s.priority[0]=a|(b<<8);s.priority[1]=c|(d<<8);
        for(unsigned code=0;code<256;++code) {
            const unsigned p=priority(s,code);
            assert(lagi::spritePixelPriority(s,code)==p);
            assert((lagi::spriteCompositeLookup(s,code)>>24)==p);
            // VDP1 wins ties, transparent pixels/priority zero never mask UI.
            for(unsigned layer=1;layer<8;++layer)
                assert(((lagi::spriteCompositeLookup(s,code)>>24)>=layer)==(p>=layer));
        }
    }
    s.priority[0]=0x405;s.priority[1]=0x507;
    // Compare the actual fast-path lookup/mask decisions with the original
    // VDP2 compositor's priority*8 + tie-order keys, including transparent UI
    // holes, absent RBG pixels, and mixed-priority LUT pixels.
    for(unsigned backing=1;backing<8;++backing)
    for(unsigned text=1;text<8;++text)
    for(unsigned condition=0;condition<4;++condition) {
        s.prina=backing<<8;s.prinb=text<<8;s.spctl=(condition<<12)|0x423;
        if(!lagi::spriteUiPrioritySupported(s))continue;
        for(unsigned code=0;code<256;++code)for(unsigned present=0;present<8;++present) {
            const auto decision=lagi::spriteCompositeLookup(s,code);
            const unsigned p=priority(s,code);
            int expected=-1,key=-1;
            const auto consider=[&](int layer,unsigned pri,unsigned tie) {
                const int k=int(pri*8+tie);
                if(pri && k>key){key=k;expected=layer;}
            };
            if(present&1)consider(0,3,4); // RBG0
            if(present&2)consider(1,backing,2);
            if(present&4)consider(2,text,0);
            consider(3,p,5); // VDP1 wins equal priority
            int actual=(present&1)?0:-1;
            const unsigned op=(decision>>((present&1)?0:8))&255;
            if(op)actual=3;
            if((present&2) && (decision>>24)<backing)actual=1;
            if((present&4) && (decision>>24)<text)actual=2;
            assert(actual==expected);
        }
    }
    s.prina=0x600;s.prinb=0x700;
    s.ccctl=0x100|0x40;s.spctl=0x2623;
    assert(!lagi::spriteUiPrioritySupported(s)); // selected sprite above UI needs its second source
    s.ccctl=0x42;s.spctl=0x1423;
    s.prinb=0x600;assert(!lagi::spriteUiPrioritySupported(s)); // native equal-priority NBG1 wins
    s.prinb=0x700;
    s.prina=0x300;assert(!lagi::spriteUiPrioritySupported(s));
    s.prina=0x600;
    s.colorWindow=1;assert(!lagi::spriteUiPrioritySupported(s));s.colorWindow=0;
    s.bgon|=1;assert(!lagi::spriteUiPrioritySupported(s));s.bgon&=~1;
    s.bgon=0x10;assert(!lagi::spriteUiPrioritySupported(s));
    puts("PASS completed VDP1 pixel priorities, native ties and conservative UI resolve eligibility");
}
