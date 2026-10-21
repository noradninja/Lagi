#include "lagi/world_map_motion.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

// Straight transcription of 06055ABC's two signed branches, kept separate
// from production's clamp formulation to catch sign/overshoot regressions.
std::int32_t native_approach(std::int32_t a,std::int32_t b,
    std::int32_t gain,std::int32_t maximum,std::int32_t minimum)
{
 auto d=std::int32_t(std::uint32_t(b)-std::uint32_t(a));
 auto step=std::int32_t((std::int64_t(d)*gain)>>16);
 if(d<0){
  auto chosen=step>=-minimum?-minimum:step;
  if(-maximum>=chosen)step=-maximum;
  else if(step>=-minimum)step=-minimum;
  d=std::int32_t(std::uint32_t(d)-std::uint32_t(step));
  if(d>0)return b;
 }else{
  auto chosen=step>=minimum?step:minimum;
  if(maximum<chosen)step=maximum;
  else if(step<minimum)step=minimum;
  d=std::int32_t(std::uint32_t(d)-std::uint32_t(step));
  if(d<0)return b;
 }
 return std::int32_t(std::uint32_t(a)+std::uint32_t(step));
}
int main(){
 using namespace lagi::worldmap;
 std::uint32_t random=0xC0FFEE;
 for(int i=0;i<100000;++i){
  random=random*1664525u+1013904223u;const auto a=std::int32_t(random);
  random=random*1664525u+1013904223u;const auto b=std::int32_t(random);
  for(auto gain:{0xA3D,0x1999,0x3333,0x51E})
   assert(map_approach(a,b,gain,0x3E7000,0x28)==native_approach(a,b,gain,0x3E7000,0x28));
 }
 for(unsigned profile=0;profile<6;++profile){
  assert(map_approach_profile(5,5,profile)==5);
  assert(map_approach_profile(0,1,profile)==1);
  assert(map_approach_profile(0,-1,profile)==-1);
  for(int sign:{-1,1}){
   std::int32_t position=0,goal=sign*100000;
   for(int frame=0;frame<2000&&position!=goal;++frame){
    const auto next=map_approach_profile(position,goal,profile);
    assert(sign>0?(next>=position&&next<=goal):(next<=position&&next>=goal));
    position=next;
   }
   assert(position==goal);
  }
 }
 for(int goal:{-100000,0,100000}){
  std::int32_t p=0,v=0;
  for(int frame=0;frame<2000&&(p!=goal||v);++frame)map_spring_axis(p,goal,v);
  assert(p==goal&&v==0);
 }
 puts("PASS WORLD easing signed branch differential, six profiles, exact arrival and accelerated/decelerated springs; original code is not executed by this fixture");
}
