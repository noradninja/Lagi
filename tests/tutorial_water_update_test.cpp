#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u16=std::uint16_t;using fixedPoint=s32;
struct {std::array<s32,3>m0_position{};std::array<s16,3>mC_rotation{};}cameraProperties2;
struct {struct {s32 m1C_heightScale=0x700000,m18_widthScale=0xB00000;}m405C;}graphicEngineStatus;
struct sBTL_T0_WaterEffect {s32 m8_offsetX=0,mC_offsetY=0;s16 m18_height2=160;};
struct Call {int kind;s32 a,b,c;};std::vector<Call>calls;
s32 atanResult=0;
s32 atan2(s32 a,s32 b){calls.push_back({0,a,b,0});return atanResult;}
s32 getCos(u32 a){calls.push_back({1,s32(a),0,0});return 0x10000+s32(a)*5;}
s32 getSin(u32 a){calls.push_back({2,s32(a),0,0});return 0x8000+s32(a)*3;}
s32 mul(s32 a,s32 b){return s32((std::int64_t(a)*b)>>16);}
s32 MTH_Mul(s32 a,s32 b){calls.push_back({3,a,b,0});return mul(a,b);}
s32 FP_Div(s32 a,s32 b){calls.push_back({4,a,b,0});assert(b);return s32((std::int64_t(a)*65536)/b);}
s32 setDividend(s32 a,s32 b,s32 c){calls.push_back({5,a,b,c});assert(c);return s32((std::int64_t(a)*b)/c);}
s32 MTH_Mul_5_6(s32 a,s32 b,s32 c){calls.push_back({6,a,b,c});return mul(a,mul(b,c));}
#include "../src/integration/lagi_tutorial_water_update.inc"
int main(){
 unsigned cases=0;
 for(s32 y:{-0x200001,-1,0,1,0x200001})for(s16 pitch:{s16(-32768),s16(-1),s16(0),s16(32767)})
 for(s16 yaw:{s16(-32768),s16(-1),s16(0),s16(32767)})for(s32 angle:{-1,0,0xFFF,0x1000}){
  cameraProperties2.m0_position[1]=y;cameraProperties2.mC_rotation={pitch,yaw,0};atanResult=angle;
  sBTL_T0_WaterEffect state;calls.clear();lagiTutorialWaterUpdate(&state);
  const unsigned sum=u16(s16(pitch+angle))&0xFFF;
  const s32 distance=s32((std::int64_t(0x200000)*65536)/(0x10000+(u16(angle)&0xFFF)*5));
  const s32 vertical=mul(distance,0x8000+sum*3),depth=mul(distance,0x10000+sum*5);
  const s32 expectedY=s32(4u-u32((std::int64_t(0x700000)*vertical)/depth));
  const s32 yawValue=mul(yaw,mul(0x3243F,0x200000));
  const s32 horizontal=s32((std::int64_t(yawValue)*65536)/0x800);
  const s32 projected=s32((std::int64_t(0xB00000)*horizontal)/0x200000);
  assert(state.mC_offsetY==expectedY && state.m8_offsetX==160-(projected%160));
  assert(calls.front().kind==0 && calls.front().a==(y>>2) && calls.front().b==0x80000);
  assert(calls.back().kind==5 && calls.back().a==0xB00000 && calls.back().b==horizontal && calls.back().c==0x200000);
  assert(calls[1].kind==1 && calls[1].a==(u16(angle)&0xFFF));
  ++cases;
 }
 std::printf("PASS tutorial water projection: %u signed height/pitch/yaw/angle cases, native helper order and signed remainder; trig helpers mocked, no device rendering\n",cases);
}
