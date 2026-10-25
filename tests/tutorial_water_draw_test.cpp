#include <array>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u16=std::uint16_t;
struct fixedPoint {s32 v;fixedPoint(s32 x=0):v(x){}s32 asS32()const{return v;}};
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((std::int64_t(a.v)*b.v)>>16);}
unsigned trigAngle;
fixedPoint getSin(unsigned a){trigAngle=a;assert(a==0 || a==0x400 || a==0x800 || a==0xC00);return a==0x400?0x10000:a==0xC00?-0x10000:0;}
fixedPoint getCos(unsigned a){assert(a==trigAngle);return a==0?0x10000:a==0x800?-0x10000:0;}
struct Cmd {u16 m0_CMDCTRL,m4_CMDPMOD,m6_CMDCOLR,m8_CMDSRCA,mA_CMDSIZE;s16 mC_CMDXA,mE_CMDYA,m10_CMDXB,m12_CMDYB,m14_CMDXC,m16_CMDYC,m18_CMDXD,m1A_CMDYD;};
struct Commands {Cmd commands[16];Cmd* data(){return commands;}Cmd* end(){return commands+16;}Cmd& operator[](unsigned i){return commands[i];}}mainContextVdp1[1];
struct Packet {unsigned m4_bucketTypes;Cmd* m6_vdp1EA;};std::array<Packet,16>packets;
struct Context {Cmd* m0_currentVdp1WriteEA;Packet* m20_pCurrentVdp1Packet;unsigned m1C=0,mC=0;};
struct Clip {u16 VDP1_X2=352;fixedPoint m14_farClipDistance{0x400000},m38_oneOverFarClip{0x200};};
struct {Clip m405C;Context m14_vdp1Context[1];}graphicEngineStatus;
struct Grid {std::array<fixedPoint,3>m64_cameraRotationTarget;}grid;
struct Overlay {Grid* m8_gridTask=&grid;}overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;}manager;
Manager* gBattleManager=&manager;
struct sBTL_T0_WaterEffect {s32 m8_offsetX=160,mC_offsetY=4;u16 m10_vdp1Base=0x1234,m12=0xA37,m14_width=0x200,m16_height=0xA0;s16 m18_height2=160,m1A_height3=80;unsigned m1C_flag=1;};
#include "../src/integration/lagi_tutorial_water_draw.inc"
void reset(){graphicEngineStatus.m14_vdp1Context[0]={mainContextVdp1[0].data(),packets.data()};}
int main(){
 for(int angle:{0,0x400,0x800,0xC00})for(int offset:{-160,0,160}){
  reset();sBTL_T0_WaterEffect self;self.m8_offsetX=offset;grid.m64_cameraRotationTarget[2]=angle<<16;
  lagiTutorialWaterDraw(&self);
  const unsigned count=unsigned((176-(offset-176))/160+1);auto& c=graphicEngineStatus.m14_vdp1Context[0];
  assert(c.mC==count && c.m1C==count);
  const int sine=angle==0x400?1:angle==0xC00?-1:0,cosine=angle==0?1:angle==0x800?-1:0;
  for(unsigned i=0;i<count;++i){
   const int left=offset-176+int(i)*160,right=left+160;
   const std::array<std::array<int,2>,4> input={{{left,84},{right,84},{right,4},{left,4}}};
   const auto& cmd=mainContextVdp1[0][i];
   const std::array<std::array<int,2>,4> out={{{cmd.mC_CMDXA,cmd.mE_CMDYA},{cmd.m10_CMDXB,cmd.m12_CMDYB},{cmd.m14_CMDXC,cmd.m16_CMDYC},{cmd.m18_CMDXD,cmd.m1A_CMDYD}}};
   for(unsigned v=0;v<4;++v){const int x=input[v][0],y=input[v][1]-4;assert(out[v][0]==x*cosine-y*sine);assert(out[v][1]==-(-x*sine+y*cosine+4));}
   assert(cmd.m0_CMDCTRL==0x1002 && cmd.m4_CMDPMOD==0x4A0 && cmd.m6_CMDCOLR==0x200 && cmd.m8_CMDSRCA==0x1234 && cmd.mA_CMDSIZE==0xA37);
   assert(packets[i].m6_vdp1EA==&mainContextVdp1[0][i]);assert(packets[i].m4_bucketTypes==u16((u32(0x3FFFFF)*u32(0x200))>>16));
  }
 }
 reset();sBTL_T0_WaterEffect self;self.m1C_flag=2;lagiTutorialWaterDraw(&self);assert(!graphicEngineStatus.m14_vdp1Context[0].mC);
 self.m1C_flag=1;self.m18_height2=0;lagiTutorialWaterDraw(&self);assert(!graphicEngineStatus.m14_vdp1Context[0].mC);
 self.m18_height2=160;graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA=mainContextVdp1[0].end();lagiTutorialWaterDraw(&self);assert(!graphicEngineStatus.m14_vdp1Context[0].mC);
 puts("PASS tutorial native water draw: tiled quad continuity, four roll quadrants, positions/material/buckets and resource/capacity guards; no device rendering");
}
