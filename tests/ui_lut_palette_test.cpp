#include "lagi/ui_lut_palette.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main(){
 std::array<std::uint8_t,32> lut{};std::array<std::uint8_t,4096> cram{};
 for(unsigned i=0;i<16;++i){lut[i*2]=0x80;lut[i*2+1]=i;}
 lagi::UiLutPalette a{},b{};
 assert(lagi::capture_ui_lut_palette(lut.data(),cram.data(),cram.size(),a));
 assert(a[0]==0x8000&&a[15]==0x800f);
 assert(lagi::capture_ui_lut_palette(lut.data(),cram.data(),cram.size(),b)&&a==b);
 lut[30]=0xfc;lut[31]=0x1f;
 assert(lagi::capture_ui_lut_palette(lut.data(),cram.data(),cram.size(),b)&&a!=b&&b[15]==0xfc1f);
 lut[0]=0x07;lut[1]=0xff;cram[4094]=0x93;cram[4095]=0x7a;
 assert(lagi::capture_ui_lut_palette(lut.data(),cram.data(),cram.size(),a)&&a[0]==0x937a);
 cram[4095]=0x7b;
 assert(lagi::capture_ui_lut_palette(lut.data(),cram.data(),cram.size(),b)&&a!=b&&b[0]==0x937b);
 assert(!lagi::capture_ui_lut_palette(lut.data(),cram.data(),4095,b));
 assert(!lagi::capture_ui_lut_palette(nullptr,cram.data(),4096,b));
 for(unsigned width:{1u,8u,13u,48u}){
  const unsigned height=3,stride=(width+7)&~7u;
  std::vector<std::uint32_t> source(width*height),destination(stride*height,0xdeadbeef);
  for(unsigned i=0;i<source.size();++i)source[i]=0xff000000|i;
  auto* allocation=destination.data();
  for(unsigned step=0;step<30;++step){
   source[0]=step;lagi::refresh_ui_texture_rows(destination.data(),source.data(),width,height);
   assert(destination.data()==allocation);
   for(unsigned y=0;y<height;++y)for(unsigned x=0;x<stride;++x)
    assert(destination[y*stride+x]==(x<width?source[y*width+x]:0xdeadbeef));
  }
 }
 puts("PASS: live direct/CRAM LUT palette changes, BE colors, unchanged-palette equality, in-place padded uploads; not GPU validation");
}
