// Execute the production 0602F610 service body with platform/math mocks.
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdio>
#include "lagi/lagi_vdp1_quad_geometry.h"
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u16=std::uint16_t;
struct fixedPoint {
 s32 value=0;fixedPoint(s32 n=0):value(n){} operator s32()const{return value;}
 s32 asS32()const{return value;}s32 getInteger()const{return value>>16;}
};
struct sVec3_FP{fixedPoint v[3];fixedPoint& operator[](unsigned i){return v[i];}const fixedPoint& operator[](unsigned i)const{return v[i];}};
using quadColor=std::array<u16,4>;
struct sBillboardSpriteParams{fixedPoint m0_halfWidth,m4_halfHeight;u16 m8_CMDPMOD,mA_CMDCOLR,mC_CMDSRCA,mE_CMDSIZE,m10_gouraud[4];};
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((std::int64_t(a.value)*b.value)>>16);}
fixedPoint MTH_Mul_5_6(fixedPoint a,fixedPoint b,fixedPoint c){return MTH_Mul(a,MTH_Mul(b,c));}
fixedPoint FP_Div(s32 n,fixedPoint d){assert(d.value);return s32((std::int64_t(n)*65536)/d.value);}
s32 setDividend(s32 a,s32 b,s32 d){assert(d);return s32((std::int64_t(a)*b)/d);}
s32 atan2(s32 y,s32 x){return s32(std::lround(std::atan2(double(y),double(x))*4096/(2*3.141592653589793)))&4095;}
fixedPoint getSin(u32 i){return s32(std::lround(std::sin((i&4095)*2*3.141592653589793/4096)*65536));}
fixedPoint getCos(u32 i){return s32(std::lround(std::cos((i&4095)*2*3.141592653589793/4096)*65536));}
void transformAndAddVecByCurrentMatrix(const sVec3_FP* p,sVec3_FP* out){*out=*p;}
struct Command{u16 m0_CMDCTRL,m4_CMDPMOD,m6_CMDCOLR,m8_CMDSRCA,mA_CMDSIZE,m1C_CMDGRA; s16 mC_CMDXA,mE_CMDYA,m10_CMDXB,m12_CMDYB,m14_CMDXC,m16_CMDYC,m18_CMDXD,m1A_CMDYD;};
struct Packet{u16 m4_bucketTypes;Command* m6_vdp1EA;};
struct Context{
 Command* m0_currentVdp1WriteEA;Packet* m20_pCurrentVdp1Packet;
 std::vector<quadColor> m14[2];std::vector<quadColor>::iterator m10;
 unsigned m1C=0,mC=0;
};
struct Clip{fixedPoint m10_nearClipDistance=0x3000,m14_farClipDistance=0x200000,m18_widthScale=288,m1C_heightScale=224,m38_oneOverFarClip=0x04000000;};
struct {Context m14_vdp1Context[2];Clip m405C;} graphicEngineStatus;
std::array<Command,1024> commands;std::array<Packet,1024> packets;std::array<float,4> depth;
void lagiRecordVdp1WorldDepth(const void* cmd,const float z[4]){assert(cmd==&commands[6]);std::copy(z,z+4,depth.begin());}

#include "collection_burst_body.inc"

void reset(){auto& c=graphicEngineStatus.m14_vdp1Context[0];commands.fill({});c.m0_currentVdp1WriteEA=&commands[6];c.m20_pCurrentVdp1Packet=packets.data();c.m14[0].assign(128,{});c.m10=c.m14[0].begin();c.mC=c.m1C=0;depth.fill(-1);}
int main(){
 reset();
 std::array<sVec3_FP,2> points={sVec3_FP{{0,0,0x50000}},sVec3_FP{{0x6000,0,0x50000}}};
 std::array<fixedPoint,2> widths={0,0x333};
 quadColor colors={0xe280,0xf39c,0xc3f0,0xfe1f};
 displayRaySegment_2Width(points,widths,0x6b0,0x31e,0x1780,&colors,8);
 const auto& cmd=commands[6];auto& ctx=graphicEngineStatus.m14_vdp1Context[0];
 assert(ctx.mC==1 && ctx.m1C==1 && ctx.m10-ctx.m14[0].begin()==1);
 assert(cmd.m0_CMDCTRL==0x1002 && cmd.m4_CMDPMOD==0x8c && cmd.mA_CMDSIZE==0x31e && cmd.m8_CMDSRCA==0x6b0 && cmd.m6_CMDCOLR==0x1780);
 assert(ctx.m14[0][0]==colors && cmd.m1C_CMDGRA==0);
 assert(depth[0]==5 && depth[1]==5 && depth[2]==5 && depth[3]==5);
 // The actual tapered burst collapses its zero-width end; preserve the
 // radius, then let the renderer retain native thin-sprite coverage.
 assert(cmd.mC_CMDXA==cmd.m10_CMDXB && cmd.mE_CMDYA==cmd.m12_CMDYB);
 assert(cmd.m14_CMDXC!=cmd.mC_CMDXA);
 lagi::NativeScreenQuad q={{{float(cmd.mC_CMDXA),float(cmd.mE_CMDYA)},
  {float(cmd.m10_CMDXB),float(cmd.m12_CMDYB)},{float(cmd.m14_CMDXC),float(cmd.m16_CMDYC)},
  {float(cmd.m18_CMDXD),float(cmd.m1A_CMDYD)}}};
 // Signed fixed-point projection retains a one-pixel end for this fixture;
 // it is a real triangle, not a paired-edge collapse. Do not broaden it.
 assert(cmd.m16_CMDYC!=cmd.m1A_CMDYD);
 const auto original=q;
 assert(!lagi::expand_native_collapsed_quad(q) && q==original);
 // Both ends behind near or beyond far reject without publishing a packet.
 reset();points[0][2]=points[1][2]=0x1000;
 displayRaySegment_2Width(points,widths,0x6b0,0x31e,0x1780,&colors,8);
 assert(ctx.mC==0);
 reset();points[0][2]=points[1][2]=0x300000;
 displayRaySegment_2Width(points,widths,0x6b0,0x31e,0x1780,&colors,8);
 assert(ctx.mC==0);
 // Crossing planes clips the actual service endpoints before division.
 reset();points[0][2]=0x1000;points[1][2]=0x50000;
 displayRaySegment_2Width(points,widths,0x6b0,0x31e,0x1780,&colors,8);
 assert(ctx.mC==1 && depth[0]==0.1875f && depth[1]==0.1875f && depth[2]==5);
 reset();points[0][2]=0x50000;points[1][2]=0x300000;
 displayRaySegment_2Width(points,widths,0x6b0,0x31e,0x1780,&colors,8);
 assert(ctx.mC==1 && depth[0]==5 && depth[2]==32 && depth[3]==32);
 std::puts("PASS: actual collection tapered-sprite emission, radius/coverage, texture/Gouraud/depth and clip branches; not Vita validation");
}
