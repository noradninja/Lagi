#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include "lagi/ui_edge_anchor.h"
using s16=std::int16_t;using u16=std::uint16_t;using s32=std::int32_t;using u32=std::uint32_t;
struct sSaturnPtr {int offset=0;bool isNull()const{return !offset;}sSaturnPtr operator+(int n)const{return {offset+n};}};
struct Allocation {u16 m4_vdp1Memory=0x1000;} allocation;
struct s_workAreaCopy {Allocation* m4_vd1Allocation=&allocation;} owner;
struct {std::array<std::uint8_t,32> bitField{};} mainGameState;
sSaturnPtr readSaturnEA(sSaturnPtr p){assert(p.offset==0x5be9||p.offset==0x5bed);return {p.offset==0x5be9?0x6000:0x6100};}
u16 readSaturnU16(sSaturnPtr p){
 if(p.offset==0x5bf1)return 0x110;if(p.offset==0x5bf3)return 0x210;
 assert((p.offset>=0x6000&&p.offset<=0x6014)||(p.offset>=0x6100&&p.offset<=0x6114));
 return u16(p.offset-0x6000+0xa00);
}
s16 readSaturnS16(sSaturnPtr p){
 switch(p.offset){case 0x5bf7:return 10;case 0x5bfb:return 20;case 0x5bff:return 30;case 0x5c03:return 40;default:assert(false);return 0;}
}
struct Command {u16 m0_CMDCTRL,m4_CMDPMOD,m6_CMDCOLR,m8_CMDSRCA,mA_CMDSIZE;s16 mC_CMDXA,mE_CMDYA;};
struct Packet {u16 m4_bucketTypes;Command* m6_vdp1EA;};
std::array<Command,32> commands{};std::array<Packet,32> packets{};
struct {struct {Command* m0_currentVdp1WriteEA=commands.data();Packet* m20_pCurrentVdp1Packet=packets.data();int m1C=0,mC=0;} m14_vdp1Context[1];struct {int m38_oneOverFarClip=512;}m405C;}graphicEngineStatus;
struct Fixed {int n;int asS32()const{return n;}};
Fixed MTH_Mul(int a,int b){return {int((std::int64_t(a)*b)>>16)};}
int anchored=0;
void lagiAnchorVdp1CommandLeft(const void* command,s16 origin){
 assert(command==&commands[graphicEngineStatus.m14_vdp1Context[0].mC]);
 assert(origin==10);++anchored;
}
#include "lagi/world_map_panels_draw.h"
#include "production.inc"
int main(){
 for(float aspect:{1.f,0.75f,0.6f}){
  const float origin=-160.f/176.f*aspect;
  const float width=96.f/176.f*aspect;
  const float shift=lagi::uiLeftEdgeShift(-160.f,aspect);
  assert(std::abs(origin+shift+1.f)<0.000001f);
  assert(std::abs((origin+width+shift)-(origin+shift)-width)<0.000001f);
 }
 using namespace lagi::worldmap;MapPanels s;
 mainGameState.bitField[5]=1;mainGameState.bitField[4]=8;
 draw_map_panels(s,{1},&owner,true);
 assert(s.slide==94&&s.frame==0&&s.direction==1);
 assert(anchored==1);
 assert(commands[0].m0_CMDCTRL==0x1000&&commands[0].m4_CMDPMOD==0xa0);
 assert(commands[0].m6_CMDCOLR==0x2200&&commands[1].m6_CMDCOLR==0x2300);
 assert(commands[0].m8_CMDSRCA==0x1a00&&commands[1].m8_CMDSRCA==0x1b00);
 assert(commands[0].mA_CMDSIZE==0x110&&commands[1].mA_CMDSIZE==0x210);
 assert(commands[0].mC_CMDXA==10&&commands[0].mE_CMDYA==-114);
 assert(commands[1].mC_CMDXA==-64&&commands[1].mE_CMDYA==-40);
 assert(packets[0].m4_bucketTypes==3&&packets[1].m6_vdp1EA==&commands[1]);
 s.selected=0;s.frame=9;draw_map_panels(s,{1},&owner,false);
 assert(s.slide==96&&s.frame==10&&commands[2].m8_CMDSRCA==0x1a14);
 draw_map_panels(s,{1},&owner,false);assert(s.slide==96&&s.frame==1);
 assert(graphicEngineStatus.m14_vdp1Context[0].mC==6);
 mainGameState.bitField[9]=0x10;mainGameState.bitField[12]=0x40;
 draw_map_panels(s,{1},&owner,true);assert(graphicEngineStatus.m14_vdp1Context[0].mC==6);
 assert(anchored==3);
 puts("PASS production WORLD panel descriptors, source offsets, placement/slide, depth packets, pulse wrap and story hiding; no GPU pixels");
}
