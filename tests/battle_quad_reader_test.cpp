#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using sSaturnPtr=const std::uint8_t*;
static unsigned readSaturnU8(sSaturnPtr p){return *p;}
static unsigned readSaturnU16(sSaturnPtr p){return unsigned(p[0])<<8|p[1];}
static std::int32_t readSaturnFP(sSaturnPtr p){return std::int32_t(std::uint32_t(p[0])<<24|std::uint32_t(p[1])<<16|std::uint32_t(p[2])<<8|p[3]);}
struct sVdp1Quad {
 unsigned m0_isLast,m1,m2_CMDCTRL,m4_CMDPMOD,m6_CMDSRCA,m8_CMDSIZE,mA_CMDCOLR;
 std::int32_t mC_width,m10_height,m14_X,m18_Y;
 bool operator==(const sVdp1Quad&)const=default;
};
#include "native.inc"
#include "service.inc"
int main(){
 std::uint32_t rng=0xAE712349;
 for(unsigned count=1;count<=128;++count){
  std::vector<std::uint8_t> bytes((count+1)*0x1C);
  for(auto& b:bytes){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;b=std::uint8_t(rng);}
  for(unsigned i=0;i<count;++i)bytes[i*0x1C]=i+1==count?1:0;
  const auto expected=referenceQuadReader(bytes.data()),actual=initVdp1Quad(bytes.data());
  assert(actual==expected && actual.size()==count);
  assert(actual.back().m0_isLast==1);
 }
 std::puts("PASS: production shared animated-quad reader matches upstream battle reader for 128 synthetic sequences, every native field, signed fixed-point values and terminal record");
}
