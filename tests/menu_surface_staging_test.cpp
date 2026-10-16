#include "lagi/lagi_menu_nbg2.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
static constexpr unsigned kVdp2TextLayerWidth=352,kVdp2TextLayerHeight=224;
static constexpr std::size_t kVdp2TextSnapshotBytes=0x80000,kVdp2CramSnapshotBytes=0x1000;
static unsigned char g_vdp2TextVram[kVdp2TextSnapshotBytes],g_vdp2Cram[kVdp2CramSnapshotBytes];
using Surface=std::array<std::uint32_t,kVdp2TextLayerWidth*kVdp2TextLayerHeight>;
static Surface backing,text,g_menuCompositionPixels;
static auto* g_vdp2MenuBackgroundPixels=backing.data();
static auto* g_vdp2TextLayerPixels=text.data();
static int g_vdp2MenuBackgroundTexture=0,g_vdp2TextLayerTexture=1;
static int g_vdp2MenuScroll[6];
static bool g_vdp2SaveLayout=false;
static lagi::MenuNbg2State g_menuNbg2;
static unsigned g_profileMenuTextDecodeUs=0,g_profileMenuBackingDecodeUs=0,g_profileMenuTransferUs=0;
static std::uint64_t sceKernelGetProcessTimeWide(){static std::uint64_t ticks=0;return ++ticks;}
static std::uint16_t readVdp2Be16(const unsigned char* p,std::size_t offset){return (std::uint16_t(p[offset])<<8)|p[offset+1];}
#include "rgb.inc"
#include "character.inc"
#include "decode.inc"
static void productionCompose(bool textOnly) {
#include "compose.inc"
}
int main() {
 std::uint32_t random=0x12345678;
 auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
 unsigned compared=0;
 for(unsigned frame=0;frame<24;++frame) {
  for(auto& byte:g_vdp2TextVram) byte=next();
  for(auto& byte:g_vdp2Cram) byte=next();
  for(auto& scroll:g_vdp2MenuScroll) scroll=int(next()%2048)-1024;
  g_vdp2SaveLayout=frame&1;
  g_menuNbg2={};
  if(frame&2) {
   g_menuNbg2.bgon=7;g_menuNbg2.chctlb=1;g_menuNbg2.pncn=0x800C;
   g_menuNbg2.mpofn=0x300;g_menuNbg2.mpab=0xE0E1;g_menuNbg2.mpcd=0xE1E0;
   g_menuNbg2.prina=0x304;g_menuNbg2.prinb=0x705;g_menuNbg2.craofa=0x666;
   if(frame>=8) {
    g_menuNbg2.scrollX=int(next()%4096)-2048;g_menuNbg2.scrollY=int(next()%4096)-2048;
    g_menuNbg2.pncn=0x8000|(next()&31);g_menuNbg2.craofa=next()&0x777;
    g_menuNbg2.wctlb=2|((frame>>2)&1);g_menuNbg2.window[0]=64;g_menuNbg2.window[2]=480;
    g_menuNbg2.window[1]=20;g_menuNbg2.window[3]=180;
    g_menuNbg2.ccctl=4;g_menuNbg2.ccrnb=next()&31;
   }
  }
  Surface expected{};
  // Reference the original direct-to-texture composition, with the actual
  // production character decoders and unchanged native priority rule.
  for(unsigned y=0;y<kVdp2TextLayerHeight;++y)for(unsigned x=0;x<kVdp2TextLayerWidth;++x) {
   auto decode=g_vdp2SaveLayout?decodeSaveMenuNbgPixel:decodeStatusMenuNbgPixel;
   auto nbg0=decode(g_vdp2SaveLayout?0x20000:0x71800,x,y,g_vdp2MenuScroll[0],g_vdp2MenuScroll[1]);
   auto nbg1=decode(g_vdp2SaveLayout?0x20800:0x71000,x,y,g_vdp2MenuScroll[2],g_vdp2MenuScroll[3]);
   expected[y*kVdp2TextLayerWidth+x]=!g_vdp2SaveLayout && lagi::menuNbg2Supported(g_menuNbg2)
    ?lagi::menuNbg2Compose(g_menuNbg2,nbg0,nbg1,decodeMenuNbg2Pixel(x,y)):(nbg0?nbg0:nbg1);
  }
  text.fill(0x12345678);g_menuCompositionPixels.fill(0xDEADBEEF);
  productionCompose(false);
  assert(backing==expected);
  for(auto pixel:text)assert(pixel==0x12345678);
  const auto savedBacking=backing;
  expected.fill(0);overlayAzelTextPlane(g_vdp2MenuScroll[4],g_vdp2MenuScroll[5],expected.data());
  g_menuCompositionPixels.fill(0xDEADBEEF);
  productionCompose(true);
  assert(text==expected && backing==savedBacking);
  compared+=2*kVdp2TextLayerWidth*kVdp2TextLayerHeight;
 }
 // Every 16-bit pattern word, including blank status names, all flips,
 // out-of-range character/palette banks and unaligned run boundaries.
 const MenuRowRaster raster;
 std::uint32_t row[33];
 for(unsigned pattern=0;pattern<65536;++pattern) {
  for(unsigned tile=0;tile<3;++tile) {
   const auto address=0x71000+tile*2;
   g_vdp2TextVram[address]=pattern>>8;g_vdp2TextVram[address+1]=pattern;
  }
  for(bool save:{false,true}) {
   const int y=(pattern>>8)&15,scroll=pattern&15;
   raster.backing(row,33,y,0x71000,scroll,0,save);
   for(int x=0;x<33;++x) {
    const auto expected=save?decodeSaveMenuNbgPixel(0x71000,x,y,scroll,0)
      :decodeStatusMenuNbgPixel(0x71000,x,y,scroll,0);
    assert(row[x]==expected);++compared;
   }
  }
 }
 std::printf("PASS: %u staged/reference pixels, status/save/Map layers, signed scroll, mutation and backing/text isolation; no Vita performance claim\n",compared);
}
