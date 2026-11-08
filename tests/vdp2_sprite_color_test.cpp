#include "../include/lagi/vdp2_sprite_color.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
    lagi::Vdp2SpriteColorState s;
    for(unsigned word=0;word<65536;++word) {
        s.rbgRatio=std::uint16_t(word);
        const auto snapshot=s;
        assert(snapshot.rbgRatio==word&&lagi::rbgColorRatio(snapshot)==word%32);
    }
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
    // Exhaust all indexed type-0 words, including every CCRS register.
    s.spctl=0x20;s.ccctl=0;s.prir=0;
    s.priority[0]=0x0201;s.priority[1]=0x0403;
    for(unsigned i=0;i<4;++i)s.ratio[i]=(2*i)|((2*i+1)<<8);
    for(unsigned code=0;code<65536;++code) {
        const auto encoded=lagi::spriteType0Metadata(0xFF123456,code);
        const unsigned alpha=encoded>>24;
        assert((encoded&0xFFFFFF)==0x123456);
        assert((alpha-129)/8==((code>>14)&3));
        assert((alpha-129)%8==((code>>11)&7));
        const auto decision=lagi::spriteCompositeLookup(s,alpha);
        assert((decision>>24)==1+((code>>14)&3));
        assert(((decision>>16)&31)==((code>>11)&7));
    }
    assert(lagi::spriteType0Metadata(0,0x6801)==0);
    // Once-per-completed-VDP1 composition is observably different from
    // applying a second-stage ratio after each overlapping primitive.
    const auto once=lagi::spriteRatioRgb555(31,31<<10,16);
    const auto twice=lagi::spriteRatioRgb555(31,
        lagi::spriteRatioRgb555(31,31<<10,16),16);
    assert(once!=twice);
    assert(lagi::nbg1DotMaterial(0u,true)==0u);
    assert(lagi::nbg1DotMaterial(0xff123456u,false)==0xff123456u);
    assert(lagi::nbg1DotMaterial(0xff123456u,true)==0x80123456u);
    // Native Camp register snapshot and the actual submitted floor polygon
    // COLR=2100 from the VDP1 command stream. This is a general Saturn
    // top/second-image selection test, not a Camp rendering exception.
    {
        lagi::Vdp2SpriteColorState camp{};
        camp.ccctl=0x0152;camp.spctl=0x1223;
        camp.priority[0]=0x0204;camp.priority[1]=0x0407;
        camp.ratio[0]=0x0010;
        camp.prir=0x0003;camp.prina=0x0600;camp.prinb=0x0700;
        camp.bgon=0x001a;camp.specialColor=0x0008;
        camp.rbgRatio=0x0010;
        assert(lagi::rbgLayerCompositeSupported(camp));
        assert(lagi::specialColorMode(camp,4)==0);
        assert((camp.ccctl&0x10)!=0); // RBG0 color calculation enabled
        assert((camp.ccctl&0x100)!=0); // additive, not ratio blending
        assert((camp.ccctl&0x200)==0); // top-image ratio selection
        // The type-3 palette word 2100 selects PRISA's second bank, i.e. 2.
        constexpr unsigned word=0x2100;
        const unsigned code=lagi::spriteType3Metadata(0xffaa2200u,word)>>24;
        assert(code==137u);
        const unsigned decision=lagi::spriteCompositeLookup(camp,code);
        assert((decision>>24)==2u); // sprite second image
        assert(((decision>>8)&0xffu)==3u); // sprite's own CC is additive
        assert((decision&0xffu)==0u); // sprite is below RBG0
        // RBG0 top image adds the selected second image in RGB555.
        assert(lagi::spriteRatioRgb555(0x001fu,0x7c00u,16)!=0);
    }
    // SFCCMD=0008 is NBG1 mode 2 and leaves RBG0 in mode 0.
    s={};s.specialColor=0x0008;s.ccctl=0x0012;
    assert(lagi::specialColorMode(s,1)==2);
    assert(lagi::specialColorMode(s,4)==0);
    s.sfcode=0x0004; // A: dot codes 4/5
    for(unsigned dot=0;dot<256;++dot) {
        const bool matches=((dot&15)==4 || (dot&15)==5);
        assert(lagi::specialFunctionDotMatch(s,1,dot)==matches);
        assert(lagi::scrollSpecialColorCalculationSelected(s,1,true,true,dot,false)==matches);
        assert(!lagi::scrollSpecialColorCalculationSelected(s,1,true,false,dot,false));
        assert(!lagi::scrollSpecialColorCalculationSelected(s,1,false,true,dot,false));
    }
    s.sfcode=0x2004;s.sfsel=2; // NBG1 selects B: dot codes A/B
    assert(lagi::specialFunctionDotMatch(s,1,10));
    assert(lagi::specialFunctionDotMatch(s,1,11));
    assert(!lagi::specialFunctionDotMatch(s,1,4));
    s.specialColor=0x0004; // NBG1 character mode
    assert(lagi::scrollSpecialColorCalculationSelected(s,1,true,true,0,false));
    assert(!lagi::scrollSpecialColorCalculationSelected(s,1,true,false,10,true));
    s.specialColor=0x000c; // NBG1 CRAM-MSB mode
    assert(lagi::scrollSpecialColorCalculationSelected(s,1,true,false,0,true));
    assert(!lagi::scrollSpecialColorCalculationSelected(s,1,true,true,10,false));
    s.pncn1=0x8000;
    assert(!lagi::nbg1AuxiliarySpecialColor(s));
    s.pncn1=0x8100;
    assert(lagi::nbg1AuxiliarySpecialColor(s));
    s.ccctl=0;
    assert(!lagi::scrollSpecialColorCalculationSelected(s,1,true,true,10,true));
    puts("PASS VDP2 sprite selection, exhaustive RGB555 ratio and type-3 selector preservation; GPU appearance pending");
}
