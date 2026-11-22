#include "lathum_rig_fixture.h"
#include "../include/lagi/native_formation_motion.h"
#include <string>
struct s_battleEngine{struct{bool m100_attackAnimationFinished=false,m2000=false;}m188_flags;sVec3_FP mC_battleCenter,m1A0_battleAutoScrollDelta;p_workArea m3D0=nullptr;};
struct s_battleGrid{sVec3_FP mE4_currentCameraReferenceCenter,mF0_currentCameraReferenceForward,m134_desiredCameraPosition,m140_desiredCameraTarget,m108_deltaCameraPosition,m114_deltaCameraTarget;};
struct sFormationIntroEffectTask:s_workAreaCopy{};
std::array<s_workAreaCopy,8> children;unsigned childCount=0;std::vector<unsigned> dialogueEAs;
template<class T>T* createSubTaskWithArg(p_workArea,sSaturnPtr data){dialogueEAs.push_back(data.m_offset);return reinterpret_cast<T*>(&children.at(childCount++));}
struct Transition{sVec3_FP* live;sVec3_FP endpoint;int frames;};std::vector<Transition> transitions;
p_workArea lagiCreateNativeVectorTransition(p_workArea,sVec3_FP* live,const sVec3_FP& endpoint,int16_t frames){transitions.push_back({live,endpoint,frames});return &children.at(childCount++);}
std::vector<std::string> events;sVec3_FP* eyePointer=nullptr; sVec3_FP* targetPointer=nullptr;
void battleEngine_restoreCameraDefault(){events.push_back("restore");}
void battleEngine_resetBattleCameraPreset(){events.push_back("preset");}
enum {m2=2,m7=7};void battleEngine_SetBattleMode(int mode){assert(mode==7||mode==2);events.push_back(mode==7?"mode7":"mode2");}
void battleEngine_enableAttackCamera(){events.push_back("enable");}
void battleEngine_setCurrentCameraPositionPointer(sVec3_FP* pos){eyePointer=pos;}
void battleEngine_setDesiredCameraPositionPointer(sVec3_FP* pos){targetPointer=pos;}
void battleEngine_resetCameraInterpolation(){events.push_back("interpolate");}
bool debugKill=false;bool readKeyboardToggle(int k){assert(k==0x81);return debugKill;}
void playSystemSoundEffect(int sound){assert(sound==0x6D);events.push_back("sound");}
struct sVdp1Quad{};std::vector<sVdp1Quad> quads(1);
const std::vector<sVdp1Quad>* lagiNativeBattleParticleQuad(sSaturnPtr p){assert(p.m_offset==0x060A3C60);return &quads;}
struct {npcFileDeleter* mC_fileBundle=&npc;}dramAllocatorEnd[32];
unsigned burstRng=0,particles=0;u32 particleWord(unsigned i){return (i*7919u+12345u)&0x7fffffff;}
u32 emergenceRandom(){return particleWord(burstRng++);}
sVec3_FP burstCenter,burstScroll;
void check(const sVec3_FP& v,int x,int y,int z){assert(v[0].asS32()==x&&v[1].asS32()==y&&v[2].asS32()==z);}
void createParticleEffect(npcFileDeleter* file,const std::vector<sVdp1Quad>* quad,const sVec3_FP* pos,sVec3_FP* velocity,sVec3_FP* acceleration,s32 scale,const void* colors,s16 lifetime){
 assert(file==&npc&&quad==&quads&&!acceleration&&!colors&&scale==0x50000);
 const auto i=particles*7;assert(burstRng==i+7);
 check(*velocity,int(particleWord(i)&0xFFF)-0x7FF,particleWord(i+2)&0xFFF,int(particleWord(i+1)&0xFFF)+burstScroll[2].asS32()-0x7FF);
 check(*pos,burstCenter[0].asS32()+int(particleWord(i+3)&0xFFFFF)-0x7FFFF,burstCenter[1].asS32()+int(particleWord(i+5)&0x3FFFF),burstCenter[2].asS32()+int(particleWord(i+4)&0xFFFFF)-0x7FFFF);
 assert(lifetime==particleWord(i+6)%60);++particles;
}
#include "damage_word.inc"
#define randomNumber emergenceRandom
#include "emergence.inc"
#undef randomNumber
struct {int m28[2]={111,222};}cameraProperties2;
void transformAndAddVec(const sVec3_FP& in,sVec3_FP& out,int matrix){assert(matrix==222);out=in;out[0]=in[0].asS32()+100;out[1]=in[1].asS32()+200;out[2]=in[2].asS32()+300;}
bool introFinished=false;bool battleEngine_isBattleIntroFinished(){return introFinished;}
p_workArea lagiCreateNativeBattleSpeech(p_workArea,s16 text,s16 duration,sSaturnPtr filename){assert(text==10&&!duration&&filename.m_offset==0x060A3F64);return &children.at(childCount++);}
#include "reaction.inc"

int main(int argc,char**argv){
 assert(argc==4);original.bytes=load(argv[1]);bundle.bytes=load(argv[2]);common=load(argv[3]);
 s_workAreaCopy owner,intro;LagiLathumRig rig;assert(lagiInitializeLathumRig(&owner,rig));
 s_battleEngine engine;engine.mC_battleCenter={1000,2000,3000};s_battleGrid grid;
 grid.m134_desiredCameraPosition={11,22,33};grid.m140_desiredCameraTarget={44,55,66};grid.m108_deltaCameraPosition={7,8,9};grid.m114_deltaCameraTarget={1,2,3};
 const sVec3_FP dragon{90,80,70};auto step=[&]{lagiUpdateLathumEmergence(&owner,rig,engine,grid,dragon);};
 // No intro child yet: no fabricated progression.
 step();assert(rig.substate==0&&events.empty());engine.m3D0=&intro;step();
 assert(intro.deleteCleared&&intro.task.finished&&!engine.m3D0&&rig.substate==1&&rig.controllerTimer==90);
 check(rig.cameraEye,1000+0x1E000,2000+0x11000,3000-0xAA12E);
 check(rig.cameraTarget,1000-0x6C000,2000-0x52000,3000+0x7C000);
 check(grid.mE4_currentCameraReferenceCenter,11,22,33);check(grid.mF0_currentCameraReferenceForward,44,55,66);check(grid.m108_deltaCameraPosition,0,0,0);check(grid.m114_deltaCameraTarget,0,0,0);
 assert(eyePointer==&rig.cameraEye&&targetPointer==&rig.cameraTarget);
 assert((events==std::vector<std::string>{"restore","preset","mode7","enable","interpolate"}));
 for(int i=0;i<89;++i)step();assert(rig.substate==1&&transitions.empty());step();
 assert(rig.substate==2&&transitions.size()==1&&transitions[0].frames==180&&transitions[0].live==&rig.cameraTarget);
 check(transitions[0].endpoint,1000+0x4000,2000+0xD0000,3000+0x74000);step();assert(!particles&&rig.substate==2);
 engine.m1A0_battleAutoScrollDelta={5,-7,9};burstCenter=engine.mC_battleCenter;burstScroll=engine.m1A0_battleAutoScrollDelta;
 rig.cameraTransition->getTask()->markFinished();step();assert(particles==100&&burstRng==700&&rig.substate==3&&rig.modelPhase==1&&rig.controllerTimer==30);
 check(rig.cameraEye,1000,2000+0x117000,3000+0xD5ED2);
 assert(transitions.size()==2&&transitions[1].frames==270);check(transitions[1].endpoint,1000+0x22000,2000+0x198000,3000+0x119E00);
 for(int i=0;i<30;++i)step();assert(rig.substate==3&&dialogueEAs.empty());step();assert(rig.substate==4&&dialogueEAs[0]==0x060A3F04);
 // Both dialogue AND camera completion are required at substate4.
 rig.dialogue->getTask()->markFinished();step();assert(rig.substate==4);rig.cameraTransition->getTask()->markFinished();step();
 assert(rig.substate==5&&transitions.size()==4&&dialogueEAs.back()==0x060A3F28);
 assert(transitions[2].live==&rig.cameraEye&&transitions[3].live==&rig.cameraTarget&&transitions[2].frames==270&&transitions[3].frames==270);
 check(transitions[2].endpoint,1000+0xE0000,2000+0x58000,3000-0x6412D);check(transitions[3].endpoint,1000,2000,3000-0x52000);
 step();assert(rig.substate==5);rig.cameraTransition->getTask()->markFinished();step();assert(rig.substate==6);check(rig.cameraEye,90,80,70);check(rig.cameraTarget,1000,2000,3000);
 step();assert(rig.mode==4);rig.dialogue->getTask()->markFinished();step();assert(rig.mode==0&&!rig.substate&&engine.m188_flags.m100_attackAnimationFinished&&events.back()=="restore");
 // Auto-scroll is applied before a waiting-state guard.
 check(rig.cameraEye,100,66,88);check(rig.cameraTarget,1010,1986,3018);
 // Debug shortcut retains the outstanding dialogue completion gate.
 {LagiLathumRig debugRig;debugRig.mode=4;debugRig.dialogue=&children[0];children[0].task.finished=false;
  s_battleEngine debugEngine;debugKill=true;lagiUpdateLathumEmergence(&owner,debugRig,debugEngine,grid,dragon);
  assert(debugRig.modelPhase==1&&debugRig.substate==6&&debugRig.mode==4&&!debugEngine.m188_flags.m100_attackAnimationFinished);
  children[0].task.finished=true;lagiUpdateLathumEmergence(&owner,debugRig,debugEngine,grid,dragon);assert(debugRig.mode==0&&debugEngine.m188_flags.m100_attackAnimationFinished);debugKill=false;}

 for(unsigned selected=0;selected<9;++selected){
  children={};childCount=0;events.clear();introFinished=false;
  LagiLathumRig reaction;reaction.mode=5;reaction.selectedGroup=u8(selected);reaction.corePositions.resize(11);
  for(int i=0;i<11;++i)reaction.corePositions[i]={i*10,i*20,i*30};
  s_battleEngine e;e.m1A0_battleAutoScrollDelta={1,2,3};sVec3_FP dragonLive{70,80,90};
  auto tick=[&]{lagiUpdateLathumReaction(&owner,reaction,e,dragonLive);};tick();assert(reaction.substate==0&&events.empty());check(reaction.cameraTarget,1,2,3);
  introFinished=true;tick();const int anchor[9]={8,3,9,10,6,0,4,5,7};
  check(reaction.cameraTarget,anchor[selected]*10+100,anchor[selected]*20+200,anchor[selected]*30+300);
  assert(reaction.substate==1&&eyePointer==&dragonLive&&targetPointer==&reaction.cameraTarget&&events[0]=="mode2");
  tick();assert(reaction.substate==1&&!reaction.dialogue);e.m188_flags.m2000=true;tick();assert(reaction.substate==2&&reaction.dialogue);
  tick();assert(reaction.substate==2&&!e.m188_flags.m100_attackAnimationFinished);reaction.dialogue->getTask()->markFinished();tick();assert(reaction.substate==3&&reaction.mode==5&&!e.m188_flags.m100_attackAnimationFinished);
  tick();assert(reaction.mode==0&&reaction.substate==0&&e.m188_flags.m100_attackAnimationFinished);
 }
 puts("PASS native Lathum emergence/reaction: authored gates, nine selected groups, shared speech/camera and 700 RNG calls");
}
