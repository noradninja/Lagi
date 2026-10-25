#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u16=std::uint16_t;
struct fixedPoint {s32 v;fixedPoint(s32 x=0):v(x){}s32 asS32()const{return v;}};
struct sVec3_FP {std::array<fixedPoint,3> v{};fixedPoint& operator[](unsigned i){return v[i];}const fixedPoint& operator[](unsigned i)const{return v[i];}};
static fixedPoint FP_Div(s32 n,fixedPoint d){assert(d.v);return s32((std::int64_t(n)*65536)/d.v);}
static fixedPoint mul(fixedPoint a,fixedPoint b){return s32((std::int64_t(a.v)*b.v)>>16);}
static fixedPoint MTH_Mul_5_6(fixedPoint a,fixedPoint b,fixedPoint c){return mul(a,mul(b,c));}
struct s_graphicEngineStatus_405C {s16 m0=242,m2=-242,m4=-326,m6=326,m8=112,mA=-112,mC=-176,mE=176;
 fixedPoint m10_nearClipDistance{65536},m14_farClipDistance{0x400000},m18_widthScale{100},m1C_heightScale{100},m38_oneOverFarClip{0x200};};
struct Cmd {u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0;
 s16 mC_CMDXA=0,mE_CMDYA=0,m10_CMDXB=0,m12_CMDYB=0,m14_CMDXC=0,m16_CMDYC=0,m18_CMDXD=0,m1A_CMDYD=0;u16 m1C_CMDGRA=0;};
static std::vector<Cmd> mainContextVdp1[1];
struct Packet {u16 m4_bucketTypes=0;Cmd* m6_vdp1EA=nullptr;};
static std::array<Packet,1024> packets;
struct Context {std::vector<Cmd>::iterator m0_currentVdp1WriteEA;Packet* m20_pCurrentVdp1Packet=nullptr;Packet* m24_vdp1Packets=nullptr;unsigned m1C=0,mC=0;};
static struct {s_graphicEngineStatus_405C m405C;Context m14_vdp1Context[1];}graphicEngineStatus;
static unsigned transforms=0,metadata=0;static s32 translateX=0;
static std::array<float,4> capturedDepth{};
static void transformAndAddVecByCurrentMatrix(const sVec3_FP* a,sVec3_FP* b){++transforms;*b=*a;(*b)[0]=s32(u32((*a)[0].v)+u32(translateX));}
static void lagiRecordVdp1WorldDepth(const void* cmd,const float z[4]){
 assert(cmd==&mainContextVdp1[0][6]);++metadata;
 for(unsigned i=0;i<4;++i){assert(std::isfinite(z[i])&&z[i]>0);capturedDepth[i]=z[i];}
}
#include "world_polygon_service.inc"
static std::array<sVec3_FP,4> reset(){
 mainContextVdp1[0].assign(1024,{});packets={};transforms=metadata=0;translateX=0;
 graphicEngineStatus.m405C={};auto& c=graphicEngineStatus.m14_vdp1Context[0];
 c={mainContextVdp1[0].begin()+6,packets.data(),packets.data(),0,0};
 return {{{{{-65536,65536,65536}}},{{{65536,65536,65536}}},{{{65536,-65536,65536}}},{{{-65536,-65536,65536}}}}};
}
int main(){
 auto points=reset();const auto original=points;
 lagiDisplayWorldPolygon(points.data(),0xFC1F,-1);
 const auto& cmd=mainContextVdp1[0][6];auto& c=graphicEngineStatus.m14_vdp1Context[0];
 assert(transforms==4 && metadata==1 && c.mC==1 && c.m1C==1);
 assert(cmd.m0_CMDCTRL==0x1004 && cmd.m4_CMDPMOD==0x5C0 && cmd.m6_CMDCOLR==0xFC1F);
 assert(!cmd.m8_CMDSRCA && !cmd.mA_CMDSIZE && !cmd.m1C_CMDGRA);
 assert(cmd.mC_CMDXA==-100 && cmd.mE_CMDYA==-100 && cmd.m14_CMDXC==100 && cmd.m16_CMDYC==100);
 assert(packets[0].m4_bucketTypes==0x200 && packets[0].m6_vdp1EA==&cmd);
 for(unsigned i=0;i<4;++i)for(unsigned a=0;a<3;++a)assert(points[i][a].v==original[i][a].v);
 for(unsigned depth:{0u,0x10000u,0x23456789u,0x7FFFFFFFu}){
  points=reset();lagiDisplayWorldPolygon(points.data(),0,u32(depth));
  assert(metadata==1 && packets[0].m4_bucketTypes==u16((depth*0x200u)>>16));
 }
 points=reset();translateX=65536;lagiDisplayWorldPolygon(points.data(),1,-1);
 assert(metadata==1 && cmd.mC_CMDXA==0 && cmd.m10_CMDXB==200);
 points=reset();points[0][2]=0;for(unsigned i=1;i<4;++i)points[i][2]=131072;
 lagiDisplayWorldPolygon(points.data(),1,-1);assert(metadata==1 && packets[0].m4_bucketTypes==0x200);
 assert(capturedDepth[0]==1 && capturedDepth[1]==2);
 for(unsigned mask:{5u,10u,15u}){
  points=reset();for(unsigned i=0;i<4;++i)if(mask&(1u<<i))points[i][2]=0;
  lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata && !c.mC);
 }
 for(unsigned i=0;i<4;++i){points=reset();points[i][2]=0x400000;lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata);}
 points=reset();for(auto& p:points)p[0]=p[0].v*8;lagiDisplayWorldPolygon(points.data(),1,-1);
 assert(metadata==1 && cmd.mC_CMDXA==-326 && cmd.m14_CMDXC==326);
 points=reset();for(auto& p:points)p[0]=0x100000;lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata);
 points=reset();for(auto& p:points)p[0]=2*65536;lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata); // All outside visible, inside guard.
 points=reset();points[1][0]=2*65536;points[2][0]=2*65536;lagiDisplayWorldPolygon(points.data(),1,-1);
 assert(metadata==1 && cmd.m10_CMDXB==200 && cmd.m14_CMDXC==200); // Partial shape retains guard-band vertices.
 points=reset();c.mC=1018;lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata && c.mC==1018);
 points=reset();c.m0_currentVdp1WriteEA=mainContextVdp1[0].end();lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata);
 points=reset();c.m20_pCurrentVdp1Packet=packets.data()+1024;lagiDisplayWorldPolygon(points.data(),1,-1);assert(!metadata);
 reset();lagiDisplayWorldPolygon(nullptr,1,-1);assert(!transforms);
 puts("PASS production native world polygon: near/far/screen clipping, original fixed-point projection, packet fields/key wrap, input ownership, depth metadata and capacity; transform mocked, device pending");
}
