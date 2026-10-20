#include "../include/lagi/vdp1_color_calculation.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 for(unsigned mode=0;mode<8;++mode) for(unsigned mesh:{0u,0x100u}) {
  const unsigned pmod=0x80u|mesh|mode;
  assert(lagi::vdp1_half_transparent(pmod)==(mode==3||mode==7));
  assert(lagi::vdp1_blend_texel(pmod,0)==0);
  const auto color=lagi::vdp1_blend_texel(pmod,0xFF123456u);
  assert((color&0xFFFFFFu)==0x123456u);
  assert((color>>24)==((mode==3||mode==7)?128u:255u));
  assert(lagi::vdp1_blend_texel(pmod,color)==color);
 }
 puts("PASS PMOD half-transparency selection, mask preservation, RGB preservation and mesh independence; device blending pending");
}
