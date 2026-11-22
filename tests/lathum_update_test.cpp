#include "lathum_rig_fixture.h"
#include "../include/lagi/native_formation_motion.h"
#include <string>
struct s_battleEngine{struct{bool m2000=false,m1000=false,m100_attackAnimationFinished=false;}m188_flags;sVec3_FP mC_battleCenter,m1A0_battleAutoScrollDelta;s8 m3B2_numBattleFormationRunning=0;};
struct s_battleGrid{};struct s_battleDragon{sVec3_FP m8_position;};
struct{int m28[2]={1,2};}cameraProperties2;
std::vector<std::string> events;std::vector<u32> rngScript;unsigned rngIndex=0;
u32 testRandom(){events.push_back("rng");return rngIndex<rngScript.size()?rngScript[rngIndex++]:1;}
void transformAndAddVec(const sVec3_FP& in,sVec3_FP& out,int matrix){assert(matrix==1||matrix==2);events.push_back(matrix==1?"current":"previous");out=in;for(unsigned a=0;a<3;++a)out[a]=in[a].asS32()+(matrix==1?10:100)*int(a+1);}
void transformAndAddVecByCurrentMatrix(sVec3_FP* in,sVec3_FP* out){*out=*in;}
bool readKeyboardToggle(int key){assert(key==0x81);return false;}
void createHitSparkEffect(p_workArea,sBattleTargetable&,int mode){assert(!mode);events.push_back("hit");}
sVec3_FP* getBattleTargetablePosition(sBattleTargetable& t){return t.m4_pPosition;}
void CreateDamageSpriteForCurrentBattleOverlay(sVec3_FP*,sVec3_FP*,s32 scale,s8 kind){assert(scale==0x40000&&kind==1);}
void battleEngine_FlagQuadrantForSafety(u8){}
void createDamageDisplayTask(p_workArea,s16,sVec3_FP*,int mode){assert(mode==1);events.push_back("display");}
int sounds=0;void playSystemSoundEffect(int sound){assert(sound==0x6C||sound==0x6E);++sounds;}
#include "damage.inc"
void testStepAnimation(s_3dModel* m){events.push_back("step");stepAnimation(m);++m->m16_previousAnimationFrame;}
int resets=0;void setAnimationFrame(s_3dModel* m,int frame){assert(!frame);++resets;m->m16_previousAnimationFrame=0;}
std::vector<sBattleTargetable*> deleted;
void deleteTargetable(sBattleTargetable* target){deleted.push_back(target);events.push_back("delete");}
bool introReady=false;bool battleEngine_isBattleIntroFinished(){return introReady;}
enum{m2=2};int modeChanges=0;void battleEngine_SetBattleMode(int value){assert(value==m2);++modeChanges;}
sVec3_FP* eye=nullptr; sVec3_FP* target=nullptr;int enabled=0,restores=0;
void battleEngine_setCurrentCameraPositionPointer(sVec3_FP* p){eye=p;}
void battleEngine_setDesiredCameraPositionPointer(sVec3_FP* p){target=p;}
void battleEngine_enableAttackCamera(){++enabled;}
void battleEngine_restoreCameraDefault(){++restores;}
#define stepAnimation testStepAnimation
#include "death.inc"
#undef stepAnimation
bool changePhase=false;u8 nextPhase=1;
void modeEvent(int n,LagiLathumRig& rig){events.push_back("mode"+std::to_string(n));if(changePhase)rig.modelPhase=nextPhase;}
void lagiSelectLathumTurn(LagiLathumRig& r,s_battleEngine&){modeEvent(0,r);}
void lagiUpdateLathumAttackOne(p_workArea,LagiLathumRig&r,s_battleEngine&,sVec3_FP&){modeEvent(1,r);}
void lagiUpdateLathumAttackTwo(p_workArea,LagiLathumRig&r,s_battleEngine&,sVec3_FP&){modeEvent(2,r);}
void lagiUpdateLathumAttackThree(p_workArea,LagiLathumRig&r,s_battleEngine&,const sVec3_FP&){modeEvent(3,r);}
void lagiUpdateLathumEmergence(p_workArea,LagiLathumRig&r,s_battleEngine&,s_battleGrid&,const sVec3_FP&){modeEvent(4,r);}
void lagiUpdateLathumReaction(p_workArea,LagiLathumRig&r,s_battleEngine&,sVec3_FP&){modeEvent(5,r);}
struct sVdp1Quad{};std::vector<sVdp1Quad> quads(1);
const std::vector<sVdp1Quad>* lagiNativeBattleParticleQuad(sSaturnPtr ea){assert(ea.m_offset==0x060A3B9C);return &quads;}
struct{npcFileDeleter* mC_fileBundle=&npc;}dramAllocatorEnd[32];
struct sParticleTask{int m8_quad=0;};int sGunShotTask_UpdateSub4(int* frame){return ++*frame;}
#define randomNumber testRandom
#include "particle_phase.inc"
#undef randomNumber
struct Particle{sVec3_FP position,velocity;int scale,phase;};std::vector<Particle> particles;
void createParticleEffect(npcFileDeleter* file,const std::vector<sVdp1Quad>* quad,const sVec3_FP* pos,sVec3_FP* velocity,sVec3_FP* acceleration,s32 scale,const void* colors,s16 lifetime){assert(file==&npc&&quad==&quads&&!acceleration&&!colors&&lifetime==105);sParticleTask p;lagiTestParticlePhase(&p,lifetime);particles.push_back({*pos,*velocity,scale,p.m8_quad});}
#define randomNumber testRandom
#define stepAnimation testStepAnimation
#include "update.inc"
#undef stepAnimation
#undef randomNumber
void check(const sVec3_FP&v,int x,int y,int z){assert(v[0].asS32()==x&&v[1].asS32()==y&&v[2].asS32()==z);}
int main(int argc,char**argv){
 assert(argc==4);original.bytes=load(argv[1]);bundle.bytes=load(argv[2]);common=load(argv[3]);s_workAreaCopy owner;LagiLathumRig rig;assert(lagiInitializeLathumRig(&owner,rig));s_battleEngine engine;s_battleGrid grid;s_battleDragon dragon;dragon.m8_position={10,20,30};engine.mC_battleCenter={1000,2000,3000};rig.localPosition={4,5,6};
 for(int i=0;i<11;++i)rig.corePositions[i]={i*100,i*200,i*300};auto tick=[&]{lagiUpdateLathumRig(&owner,rig,engine,grid,dragon);};
 // The wrapper dispatches each real controller boundary after phase work;
 // damage refresh is later than the mode callback.
 for(u8 mode=0;mode<6;++mode){rig.mode=mode;rig.modelPhase=1;rig.health=100;events.clear();rngScript={1};rngIndex=0;tick();assert(events[0]=="step"&&events[1]=="rng"&&events[2]=="mode"+std::to_string(mode)&&events[3]=="previous");check(rig.position,1004,2005,3006);}
 rig.mode=6;rig.modelPhase=0;engine.m3B2_numBattleFormationRunning=127;events.clear();tick();assert(engine.m3B2_numBattleFormationRunning==-128&&events.size()==1&&events[0]=="step");
 rig.modelPhase=1;rngScript={0,8,31};rngIndex=0;particles.clear();tick();assert(rngIndex==3&&particles.size()==1&&particles[0].scale==0x30000&&particles[0].phase==31);check(particles[0].position,710,1420,2130);check(particles[0].velocity,0,4096,0);
 rig.modelPhase=2;rngScript={0,0,5};rngIndex=0;particles.clear();const auto counter=engine.m3B2_numBattleFormationRunning;const auto steps=stepCalls;tick();assert(rngIndex==3&&particles.size()==1&&particles[0].scale==0x80000&&particles[0].phase==5&&counter==engine.m3B2_numBattleFormationRunning&&steps==stepCalls);check(particles[0].position,810,1620,2430);check(particles[0].velocity,0,4096,4096);
 rig.mode=4;rig.modelPhase=0;changePhase=true;nextPhase=1;events.clear();tick();assert(events[0]=="step"&&events[1]=="mode4"&&events[2]=="previous");rig.modelPhase=1;nextPhase=2;rngScript={1};rngIndex=0;events.clear();tick();assert(events.size()==3&&events[2]=="mode4");changePhase=false;
 // Actual damage -> death flow: life-meter ownership and all13 grouped targets.
 rig.mode=6;rig.modelPhase=1;rig.health=1;rig.selectedGroup=0;rig.substate=0;rig.lifeMeter->m31=0;rig.groups[0].targets[0].m50_flags=0xA0000;rig.groups[0].targets[0].m58=2;rngScript={1};rngIndex=0;tick();assert(rig.mode==11&&!rig.health&&!rig.substate);
 const auto previousLocal=rig.localPosition;deleted.clear();resets=0;rngIndex=0;tick();assert(rig.modelPhase==2&&rig.substate==1&&resets==1&&(rig.lifeMeter->m31&9)==9&&deleted.size()==13);check(rig.position,previousLocal[0].asS32()+1000,previousLocal[1].asS32()+2000,previousLocal[2].asS32()+3000);assert(rig.localPosition[2].asS32()==previousLocal[2].asS32()+0x2000);
 unsigned n=0;for(auto& group:rig.groups)for(auto& t:group.targets)assert(deleted[n++]==&t);
 tick();assert(rig.substate==1&&!modeChanges);introReady=true;tick();assert(rig.substate==2&&modeChanges==1);tick();assert(rig.substate==2);engine.m188_flags.m2000=true;tick();assert(rig.substate==3&&rig.controllerTimer==240&&eye==&rig.cameraEye&&target==&rig.cameraTarget&&enabled==1);
 for(int frame=0;frame<240;++frame)tick();assert(rig.substate==3&&rig.controllerTimer==0&&rig.models[0].m16_previousAnimationFrame==33);tick();assert(rig.substate==10);const auto priorRestores=restores;tick();assert(rig.substate==11&&rig.controllerTimer==15&&engine.m188_flags.m100_attackAnimationFinished&&restores==priorRestores+1);for(int frame=0;frame<14;++frame)tick();assert(!rig.hideBody);tick();assert(rig.hideBody&&!owner.task.finished&&deleted.size()==13);
 puts("PASS Lathum main Update/death: phase/mode/damage order, ambient RNG, frame33 hold, target retirement and reward/camera/timer gates");
}
