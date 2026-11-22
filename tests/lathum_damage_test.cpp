#include "lathum_rig_fixture.h"
#include <string>
struct EngineFlags {bool m1000=false;};
struct s_battleEngine {sVec3_FP mC_battleCenter;EngineFlags m188_flags;} engine;
struct {int m28[2]={100,200};} cameraProperties2;
std::vector<std::string> events;bool debugKill=false;int refreshes=0,displays=0;s16 displayed=0;
void transformAndAddVec(const sVec3_FP& in,sVec3_FP& out,int camera){assert(camera==200);out=in;out.x.value+=200;}
void transformAndAddVecByCurrentMatrix(sVec3_FP* in,sVec3_FP* out){*out=*in;out->z.value+=300;++refreshes;}
void createHitSparkEffect(p_workArea,sBattleTargetable& t,int mode){assert(!mode&&(t.m50_flags&0x80000));events.push_back("hit");}
sVec3_FP* getBattleTargetablePosition(sBattleTargetable& t){return t.m4_pPosition;}
void CreateDamageSpriteForCurrentBattleOverlay(sVec3_FP* pos,sVec3_FP* zero,s32 scale,s8 kind){assert(pos&&!zero&&scale==0x40000&&kind==1);events.push_back("sprite");}
void playSystemSoundEffect(int sound){assert(sound==0x6C);events.push_back("sound");}
void battleEngine_FlagQuadrantForSafety(u8 q){assert(q<4);events.push_back("safe"+std::to_string(q));}
bool readKeyboardToggle(int key){assert(key==0x81);return debugKill;}
void createDamageDisplayTask(p_workArea,s16 value,sVec3_FP* pos,int mode){assert(pos==&engine.mC_battleCenter&&mode==1);displayed=value;++displays;events.push_back("display");}
#include "damage.inc"
struct Scenario {
 s_workAreaCopy owner;LagiLathumRig rig;
 Scenario(){events.clear();refreshes=displays=0;debugKill=false;engine={};assert(lagiInitializeLathumRig(&owner,rig));rig.health=100;rig.mode=0;rig.corePositions.resize(11);for(int i=0;i<11;++i)rig.corePositions[i]={i*100,i*1000,i*10000};}
 void hit(unsigned g,int damage){auto& t=rig.groups[g].targets[0];t.m50_flags=0xA1234;t.m58=s16(damage);}
 void update(){lagiUpdateLathumDamage(&owner,rig,engine);}
};
int main(int argc,char** argv){
 assert(argc==4);original.bytes=load(argv[1]);bundle.bytes=load(argv[2]);common=load(argv[3]);
 for(unsigned selected=0;selected<9;++selected){
  Scenario s;s.rig.selectedGroup=u8(selected);s.hit(selected,7);s.update();
  assert(s.rig.health==93&&s.rig.pendingDamage==7&&s.rig.damagePending==1&&s.rig.mode==5);
  assert(s.rig.groupCounter==2&&s.rig.selectedHitCount==1&&meter.m31==2&&refreshes==9);
  assert(s.rig.groups[selected].state==2&&s.rig.groups[selected].targets[0].m60==20);
  assert(s.rig.groups[selected].targets[0].m50_flags==0x1234);
  const int anchor[9]={8,3,9,10,6,0,4,5,7};
  for(int g=0;g<9;++g){const auto& pos=s.rig.groups[g].position;assert(pos.x.asS32()==anchor[g]*100+200&&pos.y.asS32()==anchor[g]*1000&&pos.z.asS32()==anchor[g]*10000+300);}
  assert(events.size()==4&&events[0]=="hit"&&events[1]=="sprite"&&events[2]=="sound");
  const int quadrant[9]={0,0,0,1,1,2,2,3,3};assert(events[3]=="safe"+std::to_string(quadrant[selected]));
  s.rig.mode=1;s.hit(selected,0);s.update();assert(s.rig.mode==1&&s.rig.selectedHitCount==2);
  engine.m188_flags.m1000=true;s.update();assert(displays==1&&displayed==7&&!s.rig.pendingDamage&&!s.rig.damagePending);
 }
 {Scenario s;s.rig.selectedGroup=8;s.hit(0,4);s.update();assert(s.rig.mode==0&&s.rig.groupCounter==4&&s.rig.groups[0].state==1&&s.rig.groups[0].targets[0].m60==3);s.update();assert(s.rig.groups[0].position.y.asS32()==-0xA000+8000);}
 {Scenario s;for(unsigned g=0;g<9;++g)s.hit(g,20);s.update();assert(s.rig.health==0&&s.rig.pendingDamage==180&&s.rig.damagePending&&s.rig.mode==11&&!s.rig.substate&&displayed==180&&displays==1);assert(events.size()==37);refreshes=0;s.update();assert(!refreshes&&displays==1);}
 {Scenario s;s.hit(0,-10);engine.m188_flags.m1000=true;s.update();assert(s.rig.health==110&&displayed==-10&&!s.rig.damagePending);}
 {Scenario s;s.hit(0,0);engine.m188_flags.m1000=true;s.update();assert(s.rig.health==100&&displays==1&&displayed==0);}
 {Scenario s;s.rig.health=32767;s.hit(0,-1);s.update();assert(s.rig.mode==11&&s.rig.health==0&&displayed==-1);}
 {Scenario s;s.rig.pendingDamage=32767;s.hit(0,1);engine.m188_flags.m1000=true;s.update();assert(displayed==-32768&&s.rig.health==99);}
 {Scenario s;debugKill=true;s.rig.mode=4;s.update();assert(s.rig.health==100&&!displays);s.rig.mode=0;s.update();assert(s.rig.health==0&&s.rig.mode==11&&displays==1);}
 {Scenario s;s.rig.selectedHitCount=255;s.hit(0,1);s.rig.selectedGroup=0;s.update();assert(s.rig.selectedHitCount==0&&s.rig.mode==0);s.hit(0,1);s.update();assert(s.rig.mode==5);}
 puts("PASS Lathum grouped damage: ordering, phases, camera, repeated hits, healing, wrap, death and display gates");
}
