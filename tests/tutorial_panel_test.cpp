#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s16=std::int16_t;using u16=std::uint16_t;
struct Parent {unsigned m20_flags=0;s16 mC=212,mE=193;};
struct sTutorialDisplayTask {Parent* m0_parent;s16 mB8_frame=0;unsigned mBC_subState=0;};
struct Cmd {u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0;s16 mC_CMDXA=0,mE_CMDYA=0;};
struct Commands {Cmd commands[4];Cmd* data(){return commands;}Cmd* end(){return commands+4;}Cmd& operator[](unsigned i){return commands[i];}}mainContextVdp1[1];
struct Packet {unsigned m4_bucketTypes=99;Cmd* m6_vdp1EA=nullptr;};
std::array<Packet,4> packets;
struct Context {Cmd* m0_currentVdp1WriteEA;Packet* m20_pCurrentVdp1Packet;unsigned m1C=0,mC=0;};
struct {Context m14_vdp1Context[1];}graphicEngineStatus;
struct Alloc {u16 m4_vdp1Memory=0x1230;}allocation;
struct Bundle {Alloc* m4_vd1Allocation=&allocation;}bundle;
struct Entry {Bundle* mC_fileBundle=&bundle;};
std::vector<Entry>dramAllocatorEnd(15);
struct Overlay {unsigned getSaturnPtr(unsigned p){assert(p==0x060AADE8);return p;}}overlay;
Overlay* gCurrentBattleOverlay=&overlay;
unsigned readSaturnEA(unsigned p){assert(p==0x060AADE8 || p==0x060AADEC);return p==0x060AADE8?0x100:0x200;}
u16 readSaturnU16(unsigned p){const unsigned f=p>=0x200?1:0;const unsigned off=p-(f?0x200:0x100);assert(off==2 || off==6 || off==8 || off==10);return u16(f*0x100+off);}
#include "../src/integration/lagi_tutorial_panel.inc"
void reset(){graphicEngineStatus={};auto& c=graphicEngineStatus.m14_vdp1Context[0];c.m0_currentVdp1WriteEA=mainContextVdp1[0].data();c.m20_pCurrentVdp1Packet=packets.data();}
int main(){
 Parent p;sTutorialDisplayTask task{&p};
 for(unsigned flags=0;flags<8;++flags)for(unsigned state=0;state<4;++state)for(unsigned n=0;n<16;++n){
  reset();p.m20_flags=flags;task.mBC_subState=state;task.mB8_frame=s16(n);
  tutorialDisplay_draw(&task);
  unsigned expected=flags,next=state;
  if(state==0 && (flags&2)){expected&=~4u;next=1;}
  else if(state==1 && !(flags&2)){expected|=4;next=0;}
  assert(p.m20_flags==expected && task.mBC_subState==next && task.mB8_frame==n+1);
  auto& c=graphicEngineStatus.m14_vdp1Context[0];
  assert(c.mC==unsigned(bool(flags&1)) && c.m1C==c.mC);
  if(flags&1){
   const unsigned f=(expected&4) && ((n+1)%8<4)?1:0;auto& cmd=mainContextVdp1[0][0];
   assert(cmd.m0_CMDCTRL==0x1000 && cmd.m4_CMDPMOD==f*0x100+10);
   assert(cmd.m6_CMDCOLR==0x1230+f*0x100+6 && cmd.m8_CMDSRCA==0x1230+f*0x100+2);
   assert(cmd.mA_CMDSIZE==f*0x100+8 && cmd.mC_CMDXA==20 && cmd.mE_CMDYA==49);
   assert(packets[0].m4_bucketTypes==0 && packets[0].m6_vdp1EA==&cmd);
  }
 }
 reset();p.m20_flags=5;task.mBC_subState=2;task.mB8_frame=32767;tutorialDisplay_draw(&task);assert(task.mB8_frame==0);
 reset();p.m20_flags=1;task.mB8_frame=32767;tutorialDisplay_draw(&task);assert(task.mB8_frame==-32768);
 reset();graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA=mainContextVdp1[0].data()+4;
 tutorialDisplay_draw(&task);assert(graphicEngineStatus.m14_vdp1Context[0].mC==0);
 reset();bundle.m4_vd1Allocation=nullptr;tutorialDisplay_draw(&task);assert(graphicEngineStatus.m14_vdp1Context[0].mC==0);
 puts("PASS native tutorial panel: 512 flag/state/frame cases, blink handshake, native sprite fields/base/position, wrap and capacity/resource guards; no device rendering");
}
