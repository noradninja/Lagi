#include "../include/lagi/vdp2_sprite_color.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
    lagi::Vdp2SpriteColorState s;
    s.ccctl=0x40;s.spctl=0x1403;s.priority[0]=0x404;s.ratio[0]=0x1210;s.ratio[1]=0x14;
    assert(lagi::spritePriority(s,0)==4&&lagi::spriteColorRatio(s,0)==16);
    assert(lagi::spriteColorRatio(s,1)==18&&lagi::spriteColorRatio(s,2)==20);
    for(unsigned mode=0;mode<4;++mode)for(unsigned p=0;p<8;++p)for(bool msb:{false,true}) {
        s.spctl=(mode<<12)|(4<<8)|3;
        assert(lagi::spriteColorCalculationSelected(s,p,msb)==
            (mode==0?p<=4:mode==1?p==4:mode==2?p>=4:msb));
        assert(!lagi::spriteColorCalculationSelected(s,p,msb,false));
    }
    s.ccctl=0;assert(!lagi::spriteColorCalculationSelected(s,4,true));
    for(unsigned r=0;r<32;++r)for(unsigned a=0;a<32;++a)for(unsigned b=0;b<32;++b) {
        unsigned expected=(a*(31-r)+b*(r+1))>>5;
        assert(lagi::spriteRatioRgb555(a|(a<<5)|(a<<10),b|(b<<5)|(b<<10),r)==
            (0x8000|expected|(expected<<5)|(expected<<10)));
    }
    puts("PASS VDP2 sprite register selection and exhaustive RGB555 ratio oracle; rendering not enabled");
}
