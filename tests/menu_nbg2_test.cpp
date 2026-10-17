#include "lagi/lagi_menu_nbg2.h"
#include <cassert>
#include <cstdio>
#include <cstring>
static constexpr std::size_t kVdp2TextSnapshotBytes=0x80000,kVdp2CramSnapshotBytes=0x1000;
static unsigned char g_vdp2TextVram[kVdp2TextSnapshotBytes],g_vdp2Cram[kVdp2CramSnapshotBytes];
static lagi::MenuNbg2State g_menuNbg2;
static std::uint16_t readVdp2Be16(const unsigned char* p,std::size_t offset){return (std::uint16_t(p[offset])<<8)|p[offset+1];}
#include "menu_nbg2_rgb_production.inc"
#include "menu_nbg2_character_production.inc"
#include "menu_nbg2_decode_production.inc"
static void word(unsigned char* p,std::size_t offset,unsigned value){p[offset]=value>>8;p[offset+1]=value&255;}
static std::uint32_t gray(unsigned v){v=v*255/31;return 0xFF000000|v|(v<<8)|(v<<16);}
int main()
{
    auto& state=g_menuNbg2;
    state.bgon=7;state.chctlb=1;state.pncn=0x800C;
    state.mpofn=0x300;state.mpab=0xE0E1;state.mpcd=0xE1E0;
    state.prina=0x304;state.prinb=0x705;state.craofa=0x666;
    assert(lagi::menuNbg2Supported(state));
    assert(lagi::menuNbg2PatternAddress(state,0,0)==0x70800);
    assert(lagi::menuNbg2PatternAddress(state,512,0)==0x70000);
    assert(lagi::menuNbg2PatternAddress(state,0,512)==0x70000);
    assert(lagi::menuNbg2PatternAddress(state,512,512)==0x70800);
    state.scrollX=-1;state.scrollY=-1;
    assert(lagi::menuNbg2PatternAddress(state,0,0)==0x70FFE);
    state.scrollX=0;state.scrollY=0;
    const unsigned colors[]={0x001F,0x03E0,0x7C00,0x7FFF};
    for(unsigned cell=0;cell<4;++cell) {
        std::memset(g_vdp2TextVram+0x60080+cell*32,(cell+1)*0x11,32);
        word(g_vdp2Cram,(0x640+cell+1)*2,colors[cell]);
    }
    unsigned decoded=0;
    for(unsigned flip=0;flip<4;++flip) {
        word(g_vdp2TextVram,0x70800,0x4001|(flip<<10));
        for(int y=0;y<16;++y)for(int x=0;x<16;++x) {
            const int sx=(flip&1)?15-x:x,sy=(flip&2)?15-y:y;
            const unsigned cell=(sy>>3)*2+(sx>>3);
            assert(decodeMenuNbg2Pixel(x,y)==vdp2Rgb555ToAbgr(colors[cell]));
            ++decoded;
        }
    }
    g_vdp2TextVram[0x60080]=0;
    word(g_vdp2TextVram,0x70800,0x4001);
    assert(decodeMenuNbg2Pixel(0,0)==0); // Transparent dot, not opaque black.
    state.window[0]=192;state.window[1]=32;state.window[2]=575;state.window[3]=223;
    for(unsigned area=0;area<2;++area) {
        state.wctlb=2|area;
        for(int y=30;y<226;++y)for(int x=94;x<290;++x) {
            const bool inside=x>=96 && x<=287 && y>=32 && y<=223;
            assert(lagi::menuNbg2VisibleAt(state,x,y)==(area?inside:!inside));
        }
    }
    state.wctlb=0;
    state.ccctl=4;
    unsigned blends=0;
    for(unsigned ratio=0;ratio<32;++ratio)for(unsigned top=0;top<32;++top)for(unsigned second=0;second<32;++second) {
        state.ccrnb=static_cast<std::uint16_t>(ratio);
        assert(lagi::menuNbg2Blend(state,gray(top),gray(second))==gray((top*(31-ratio)+second*(ratio+1))>>5));
        ++blends;
    }
    unsigned mixerPixels=0;
    state.prina=0x304;state.prinb=5;state.bgon=7;
    for(unsigned ratio=0;ratio<32;++ratio) {
        state.ccrnb=ratio;
        const lagi::MenuNbg2Mixer mixer(state);
        for(unsigned color=0;color<32768;++color) {
            const auto a=vdp2Rgb555ToAbgr(color);
            const auto b=vdp2Rgb555ToAbgr((color*109+ratio*73)&32767);
            assert(mixer.compose(b,0,a)==lagi::menuNbg2Compose(state,b,0,a));
            ++mixerPixels;
        }
        assert(mixer.compose(0,0,0)==0);
        assert(mixer.compose(0,0,gray(31))==lagi::menuNbg2Compose(state,0,0,gray(31)));
    }
    state.ccctl=0;
    const std::uint32_t pixel[]={0xFF0000FF,0xFF00FF00,0xFFFF0000};
    unsigned priorities=0;
    for(unsigned p0=0;p0<8;++p0)for(unsigned p1=0;p1<8;++p1)for(unsigned p2=1;p2<8;++p2)
    for(unsigned present=0;present<8;++present) {
        state.prina=static_cast<std::uint16_t>(p0|(p1<<8));state.prinb=static_cast<std::uint16_t>(p2);
        unsigned bestPriority=0;std::uint32_t expected=0;
        // Reverse traversal: equal priorities favor NBG0, then1, then2.
        const unsigned priority[]={p0,p1,p2};
        for(int layer=2;layer>=0;--layer)if((present&(1<<layer)) && priority[layer] && priority[layer]>=bestPriority) {
            bestPriority=priority[layer];expected=pixel[layer];
        }
        assert(lagi::menuNbg2Compose(state,present&1?pixel[0]:0,present&2?pixel[1]:0,present&4?pixel[2]:0)==expected);
        const lagi::MenuNbg2Mixer mixer(state);
        assert(mixer.compose(present&1?pixel[0]:0,present&2?pixel[1]:0,present&4?pixel[2]:0)==expected);
        ++priorities;
    }
    state.prina=0x304;state.prinb=5;
    auto invalid=state;invalid.prinb=0;assert(!lagi::menuNbg2Supported(invalid));
    invalid=state;invalid.chctlb=3;assert(!lagi::menuNbg2Supported(invalid));
    invalid=state;invalid.pncn=0xC00C;assert(!lagi::menuNbg2Supported(invalid));
    invalid=state;invalid.plsz=0x10;assert(!lagi::menuNbg2Supported(invalid));
    invalid=state;invalid.wctlb=4;assert(!lagi::menuNbg2Supported(invalid));
    invalid=state;invalid.bgon|=0x400;assert(!lagi::menuNbg2Supported(invalid));
    invalid=state;invalid.ccctl=0x100;assert(!lagi::menuNbg2Supported(invalid));
    std::printf("PASS: production NBG2 decode %u pixels/flips, window edges, %u RGB555 blends, %u priority cases; not GPU execution\n",decoded,blends,priorities);
    std::printf("PASS: prepared Map mixer matches original across %u colored RGB555/ratio cases and transparent/tied-priority paths\n",mixerPixels);
}
