#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u32=std::uint32_t;
struct File {} a5,a3;
File* gFLD_A5=&a5;
struct sSaturnPtr {u32 m_offset;File* m_file;};
struct Fade {u32 m0_color;};
struct {Fade m0_fade0{0x1234},m24_fade1{0x5678};unsigned m_4C=0,m_4D=0,m_48=0;} g_fadeControls;
struct {struct {unsigned m112_CLOFSL=0;} m20_registers[2];bool m_isDirty=false;} vdp2Controls;
struct Call {Fade* fade;u32 from;s32 to,duration;};
std::vector<Call> calls;
u32 convertColorToU32ForFade(u32 color){return color^0x1357;}
void fadePalette(Fade* fade,u32 from,s32 to,s32 duration){calls.push_back({fade,from,to,duration});}
#include "production.inc"
int main(){
 s32 result=99;
 for(auto target:{sSaturnPtr{0x0606FA5C,&a3},sSaturnPtr{0x0606FAB4,nullptr},sSaturnPtr{0x060728E0,&a5}}){
  assert(!lagiAzelTryGarilScriptAction(target,15,result)&&result==99&&calls.empty());
 }
 gFLD_A5=nullptr;assert(!lagiAzelTryGarilScriptAction({0x0606FA5C,&a5},15,result));gFLD_A5=&a5;
 for(const auto pair:{std::array<unsigned,2>{0,0},{128,127},{127,128},{255,0},{0,255}})
 for(u32 address:{0x0606FA5Cu,0x0606FAB4u})
 for(s32 duration:{0,1,15,-1,INT32_MAX,INT32_MIN}){
  calls.clear();result=99;g_fadeControls.m_4C=pair[0];g_fadeControls.m_4D=pair[1];
  g_fadeControls.m_48=0xFFFF;
  for(auto& r:vdp2Controls.m20_registers)r.m112_CLOFSL=3;vdp2Controls.m_isDirty=false;
  assert(lagiAzelTryGarilScriptAction({address,&a5},duration,result)&&result==0&&calls.size()==2);
  const bool selected=s8(pair[1])>=s8(pair[0]);
  assert(vdp2Controls.m_isDirty==selected);
  for(auto& r:vdp2Controls.m20_registers)assert(r.m112_CLOFSL==(selected?0x10u:3u));
  assert(calls[0].fade==&g_fadeControls.m0_fade0&&calls[1].fade==&g_fadeControls.m24_fade1);
  for(const auto& call:calls){
   assert(call.duration==s32(u32(duration)<<1));
   assert(call.from==(address==0x0606FA5C?(call.fade->m0_color^0x1357):0x8000));
   assert(call.to==(address==0x0606FA5C?0x8000:-1));
  }
 }
 puts("PASS A5 fade-action ownership, foreign dispatch isolation, signed group selection, two-channel order, colors and low-word duration; native fade service mocked");
}
