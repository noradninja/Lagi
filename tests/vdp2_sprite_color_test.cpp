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
    for(unsigned code=0;code<0x8000;++code) {
        const auto encoded=lagi::spriteType3Metadata(0xFF123456,code);
        assert((encoded&0xFFFFFF)==0x123456);
        const unsigned selectors=(encoded>>24)-129;
        assert(selectors/8==((code>>13)&3));
        assert(selectors%8==((code>>11)&3));
        assert((encoded>>24)>=129&&(encoded>>24)<=156);
    }
    assert(lagi::spriteType3Metadata(0,0x6801)==0);
    // Once-per-completed-VDP1 composition is observably different from
    // applying a second-stage ratio after each overlapping primitive.
    const auto once=lagi::spriteRatioRgb555(31,31<<10,16);
    const auto twice=lagi::spriteRatioRgb555(31,
        lagi::spriteRatioRgb555(31,31<<10,16),16);
    assert(once!=twice);
    puts("PASS VDP2 sprite selection, exhaustive RGB555 ratio and type-3 selector preservation; GPU appearance pending");
}
