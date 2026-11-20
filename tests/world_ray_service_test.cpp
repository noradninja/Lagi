#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u16=std::uint16_t;
struct fixedPoint {s32 v;fixedPoint(s32 x=0):v(x){}s32 asS32()const{return v;}};
using sVec3_FP=std::array<fixedPoint,3>;
using sScreenQuad3=std::array<std::array<s32,3>,4>;
struct s_graphicEngineStatus_405C {s16 m0=242,m2=-242,m4=-326,m6=326,m8=112,mA=-112,mC=-176,mE=176;fixedPoint m10_nearClipDistance{65536},m14_farClipDistance{0x400000},m38_oneOverFarClip{0x200};};
struct Cmd {u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0;
 s16 mC_CMDXA=0,mE_CMDYA=0,m10_CMDXB=0,m12_CMDYB=0,m14_CMDXC=0,m16_CMDYC=0,m18_CMDXD=0,m1A_CMDYD=0;u16 m1C_CMDGRA=0;};
static std::vector<Cmd> mainContextVdp1[1];
struct Packet {u16 m4_bucketTypes=0;Cmd* m6_vdp1EA=nullptr;};
static std::array<Packet,1024> packets;
struct Context {std::vector<Cmd>::iterator m0_currentVdp1WriteEA;Packet* m20_pCurrentVdp1Packet=nullptr;Packet* m24_vdp1Packets=nullptr;unsigned m1C=0,mC=0;};
static struct {s_graphicEngineStatus_405C m405C;Context m14_vdp1Context[1];}graphicEngineStatus;
static unsigned transforms=0,projections=0,metadata=0;
static float capturedDepth[4];static std::array<sVec3_FP,2> projectedView;
static sScreenQuad3 nextScreen={{{-10,10,0},{10,10,0},{10,-10,0},{-10,-10,0}}};
static void transformAndAddVecByCurrentMatrix(const sVec3_FP* a,sVec3_FP* b){++transforms;*b=*a;}
static void lagiRecordVdp1WorldDepth(const void* cmd,const float z[4]) {
 assert(cmd==&mainContextVdp1[0][6]);++metadata;
 for(unsigned i=0;i<4;++i){assert(std::isfinite(z[i]) && z[i]>0);capturedDepth[i]=z[i];}
}
#include "world_service.inc"
static void reset() {
 mainContextVdp1[0].assign(1024,{});packets={};transforms=projections=metadata=0;
 graphicEngineStatus.m405C={};auto& c=graphicEngineStatus.m14_vdp1Context[0];
 c={mainContextVdp1[0].begin()+6,packets.data(),packets.data(),0,0};
 nextScreen={{{-10,10,0},{10,10,0},{10,-10,0},{-10,-10,0}}};
}
int main() {
 std::array<sVec3_FP,2> line{};line[0][2]=65536;line[1][2]=196608;
 for(unsigned mode:{0u,0x100u,0xA0u})for(unsigned source:{0u,0x1234u}) {
  reset();displayRaySegmentFromWorldSpace(line.data(),42,u16(source),0x428,0xABCD,u16(mode));
  const auto& c=graphicEngineStatus.m14_vdp1Context[0];const auto& cmd=mainContextVdp1[0][6];
  assert(transforms==2 && projections==1 && metadata==1 && c.mC==1 && c.m1C==1);
  assert(cmd.m0_CMDCTRL==0x1002 && cmd.m4_CMDPMOD==(0x480|mode));
  assert(cmd.m8_CMDSRCA==source && cmd.mA_CMDSIZE==0x428 && cmd.m6_CMDCOLR==0xABCD && !cmd.m1C_CMDGRA);
  assert(cmd.mC_CMDXA==-10 && cmd.mE_CMDYA==-10 && cmd.m14_CMDXC==10 && cmd.m16_CMDYC==10);
  assert(packets[0].m4_bucketTypes==0x600 && packets[0].m6_vdp1EA==&cmd);
  assert(c.m0_currentVdp1WriteEA==mainContextVdp1[0].begin()+7 && c.m20_pCurrentVdp1Packet==packets.data()+1);
  assert(capturedDepth[0]==1 && capturedDepth[1]==3 && capturedDepth[2]==3 && capturedDepth[3]==1);
 }
 reset();line[0][0]=-65536;line[1][0]=65536;line[0][2]=0;line[1][2]=131072;
 displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);
 assert(metadata==1 && projectedView[0][0].asS32()==0 && projectedView[0][2].asS32()==65536);
 assert(line[0][2].asS32()==0); // Producer endpoints are not mutated.
 reset();line[1][2]=0;displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata && !graphicEngineStatus.m14_vdp1Context[0].mC);
 reset();line[0][2]=65536;line[1][2]=0x400000;displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata);
 reset();line[1][2]=196608;nextScreen={{{-500,10,0},{500,10,0},{500,-10,0},{-500,-10,0}}};
 displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(metadata==1);
 assert(mainContextVdp1[0][6].mC_CMDXA==-326 && mainContextVdp1[0][6].m14_CMDXC==326);
 assert(capturedDepth[0]>1 && capturedDepth[0]<3 && capturedDepth[1]>1 && capturedDepth[1]<3);
 reset();nextScreen={{{-500,10,0},{-400,10,0},{-400,-10,0},{-500,-10,0}}};
 displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata);
 reset();nextScreen={{{200,10,0},{250,10,0},{250,-10,0},{200,-10,0}}};
 displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata);
 reset();nextScreen={{{-10,10,0},{250,10,0},{250,-10,0},{-10,-10,0}}};
 displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(metadata==1 && mainContextVdp1[0][6].m10_CMDXB==250);
 reset();auto& c=graphicEngineStatus.m14_vdp1Context[0];c.mC=1018;
 displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata && c.mC==1018);
 reset();c.m0_currentVdp1WriteEA=mainContextVdp1[0].end();displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata);
 reset();c.m20_pCurrentVdp1Packet=packets.data()+1024;displayRaySegmentFromWorldSpace(line.data(),42,1,2,3,0);assert(!metadata);
 reset();displayRaySegmentFromWorldSpace(nullptr,42,1,2,3,0);assert(!transforms);
 puts("PASS production world-ray service: native depth/screen clipping, exact unshaded packets/keys, source-zero behavior, depth metadata and capacity guards; projection/transform services mocked, no device rendering");
}
