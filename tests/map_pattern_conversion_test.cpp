#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
struct Exploration {unsigned y;unsigned calls=0;};
bool explored(void* opaque,unsigned x,unsigned y){
 auto& context=*static_cast<Exploration*>(opaque);
 assert(x==context.calls%16 && y==context.y-context.calls/16);
 ++context.calls;return (x+y)%2==0;
}
int main(){
 std::array<std::uint8_t,0x802> input{},output{};
 for(unsigned i=0;i<0x800;i+=2){input[i]=0xF0;input[i+1]=std::uint8_t(i/2);}
 for(bool owned:{false,true})for(unsigned firstY:{15u,31u}){
  output.fill(0xCD);Exploration context{firstY};
  assert(lagiAzelConvertMapPatterns(input.data(),0x800,output.data(),0x800,owned,firstY,explored,&context));
  assert(context.calls==256 && output[0x800]==0xCD && output[0x801]==0xCD);
  for(unsigned row=0;row<16;++row)for(unsigned x=0;x<16;++x)
  for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx){
   unsigned i=row*0x80+x*4+dy*0x40+dx*2;
   bool visible=(x+firstY-row)%2==0;
   std::uint16_t expected=visible||owned?std::uint16_t((0xF000|input[i+1])+(visible?0x4000:0x7000)):0x238;
   assert(((unsigned(output[i])<<8)|output[i+1])==expected);
  }
 }
 Exploration context{15};output.fill(0xCD);
 assert(!lagiAzelConvertMapPatterns(input.data(),0x7FF,output.data(),0x800,true,15,explored,&context));
 assert(!context.calls && output[0]==0xCD);
 puts("PASS: native PNB 2x2 block conversion, both halves/ownership states, exploration order, blanking, wrapping and bounds; no renderer test");
}
