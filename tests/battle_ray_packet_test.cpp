#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using s32=std::int32_t;using s16=std::int16_t;using u16=std::uint16_t;
using quadColor=std::array<u16,4>;
using sScreenQuad3=std::array<std::array<s32,3>,4>;
struct fixedPoint{s32 raw; s32 asS32()const{return raw;}};
using sVec3_FP=std::array<fixedPoint,3>;
struct s_vdp1Command {
 u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0;
 s16 mC_CMDXA=0,mE_CMDYA=0,m10_CMDXB=0,m12_CMDYB=0,m14_CMDXC=0,m16_CMDYC=0,m18_CMDXD=0,m1A_CMDYD=0;
 u16 m1C_CMDGRA=0;
 bool operator==(const s_vdp1Command&)const=default;
};
struct Packet{unsigned m4_bucketTypes=0;s_vdp1Command* m6_vdp1EA=nullptr;};
static std::vector<s_vdp1Command> commands(1024);
static std::array<Packet,1024> packets;
struct Context {
 std::vector<s_vdp1Command>::iterator m0_currentVdp1WriteEA;
 std::vector<quadColor> m14[1];std::vector<quadColor>::iterator m10;
 Packet* m24_vdp1Packets;Packet* m20_pCurrentVdp1Packet;
 unsigned m1C=0,mC=0;
};
static struct{Context m14_vdp1Context[1];int m405C=0;}graphicEngineStatus;
static unsigned metadataCalls=0;static float capturedZ[4];
static void lagiRecordVdp1WorldDepth(const void* command,const float z[4]){
 assert(command==&commands[6]);++metadataCalls;for(unsigned i=0;i<4;++i)capturedZ[i]=z[i];
}
#include "native.inc"
#include "service.inc"
static bool projectVisible=true;
static unsigned transforms=0,projections=0;
static void transformAndAddVecByCurrentMatrix(const sVec3_FP* source,sVec3_FP* destination){
 ++transforms;*destination=*source;(*destination)[2].raw+=65536;
}
static int rayComputeDisplayMatrix_fixedWidth(std::array<sVec3_FP,2>& view,int width,int& clip,sScreenQuad3& screen){
 ++projections;assert(width==42 && &clip==&graphicEngineStatus.m405C);
 assert(view[0][2].raw==131072 && view[1][2].raw==262144);screen={};return projectVisible;
}
static int rayComputeDisplayMatrix_2Width(std::array<sVec3_FP,2>& view,std::array<fixedPoint,2>& widths,int& clip,sScreenQuad3& screen){
 assert(widths[0].raw==42 && widths[1].raw==84);
 return rayComputeDisplayMatrix_fixedWidth(view,42,clip,screen);
}
#include "wrapper.inc"
static void reset(){
 commands.assign(1024,{});packets={};metadataCalls=0;
 auto& c=graphicEngineStatus.m14_vdp1Context[0];c.m14[0].assign(1024,{});
 c.m0_currentVdp1WriteEA=commands.begin()+6;c.m10=c.m14[0].begin()+7;
 c.m24_vdp1Packets=packets.data();c.m20_pCurrentVdp1Packet=packets.data();c.m1C=c.mC=0;
}
int main(){
 std::uint32_t rng=0xB1237809;
 auto next=[&](){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;};
 for(unsigned trial=0;trial<1024;++trial){
  sScreenQuad3 screen;for(auto& point:screen)for(auto& coordinate:point)coordinate=static_cast<s32>(next());
  quadColor colors;for(auto& color:colors)color=static_cast<u16>(next());
  fixedPoint depth{static_cast<s32>(next())};
  const u16 address=next(),size=next(),color=next();const s32 mode=next()&0x7F;
  reset();referenceSend(screen,depth,address,size,color,&colors,mode);
  const auto expected=commands[6];
  const auto expectedColor=graphicEngineStatus.m14_vdp1Context[0].m14[0][7];
  reset();sendRaySegmentToVdp1(screen,depth,address,size,color,&colors,mode);
  const auto& c=graphicEngineStatus.m14_vdp1Context[0];
  assert(commands[6]==expected && c.m14[0][7]==expectedColor);
  assert(c.mC==1 && c.m1C==1 && c.m10==c.m14[0].begin()+8);
  assert(c.m0_currentVdp1WriteEA==commands.begin()+7 && c.m20_pCurrentVdp1Packet==packets.data()+1);
  assert(packets[0].m4_bucketTypes==0 && packets[0].m6_vdp1EA==&commands[6]);
  assert(metadataCalls==1);for(float z:capturedZ)assert(z==float(depth.asS32())/65536.0f);
 }
 reset();sScreenQuad3 screen{};fixedPoint depth{65536};quadColor colors{};
 auto& c=graphicEngineStatus.m14_vdp1Context[0];
 sendRaySegmentToVdp1(screen,depth,0,0,0,nullptr,0);assert(c.mC==0 && !metadataCalls);
 c.mC=1018;sendRaySegmentToVdp1(screen,depth,0,0,0,&colors,0);assert(c.mC==1018 && !metadataCalls);
 c.mC=0;c.m10=c.m14[0].end();sendRaySegmentToVdp1(screen,depth,0,0,0,&colors,0);assert(!metadataCalls);
 c.m10=c.m14[0].begin();c.m20_pCurrentVdp1Packet=packets.data()+1024;
 sendRaySegmentToVdp1(screen,depth,0,0,0,&colors,0);assert(c.mC==0 && !metadataCalls);
 reset();std::array<sVec3_FP,2> endpoints{};endpoints[0][2].raw=65536;endpoints[1][2].raw=196608;
 displayRaySegment(endpoints,42,1,2,3,&colors,0);
 assert(transforms==2 && projections==1 && metadataCalls==2 && c.mC==1);
 assert(capturedZ[0]==2 && capturedZ[1]==4 && capturedZ[2]==4 && capturedZ[3]==2);
 reset();projectVisible=false;displayRaySegment(endpoints,42,1,2,3,&colors,0);
 assert(c.mC==0 && !metadataCalls && transforms==4 && projections==2);
 reset();projectVisible=true;std::array<fixedPoint,2> widths={fixedPoint{42},fixedPoint{84}};
 lagiDisplayWorldStrip(endpoints,widths,1,2,3,&colors,8);
 assert(c.mC==1 && metadataCalls==2 && transforms==6 && projections==3);
 assert(capturedZ[0]==2 && capturedZ[1]==4 && capturedZ[2]==4 && capturedZ[3]==2);
 reset();projectVisible=false;lagiDisplayWorldStrip(endpoints,widths,1,2,3,&colors,8);
 assert(c.mC==0 && !metadataCalls && transforms==8 && projections==4);
 std::puts("PASS: native projection body preserved; 1024 packet/Gouraud/coordinate/priority comparisons, world/view handoff and bounded-buffer guards; no GPU execution");
}
