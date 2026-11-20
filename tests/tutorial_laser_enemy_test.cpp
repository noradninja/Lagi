#define main modelLifecycleFixtureMain
#include "native_battle_models_test.cpp"
#undef main
#include "lagi/native_formation_motion.h"
#include "lagi/native_tutorial_laser_enemy.h"
#include "../src/integration/lagi_battle_model_damage.inc"
#include "../src/integration/lagi_tutorial_model_motion.inc"
struct npcFileDeleter {int id=14;s_fileBundle* m0_fileBundle=nullptr;};
static unsigned debrisCalls=0;
static LagiTutorialLaserEnemyView* liveActor=nullptr;
static void createBoneDebris(s_workAreaCopy* owner,npcFileDeleter* resources,s_3dModel* model,
 sVec3_FP* pos,sVec3_FP* rot,sVec3_FP* velocity,sVec3_FP* acceleration,
 int gravity,int mask,int a,int b,int16_t bounce,int scale,int16_t life){
 assert(owner==liveActor->damage.owner&&resources==liveActor->resources);
 assert(model==liveActor->damage.models.data()&&pos==&liveActor->motion.worldPosition&&rot==&liveActor->motion.worldRotation);
 assert(!velocity&&!acceleration&&gravity==-44&&mask==0x1fffff&&a==0&&b==0&&bounce==0&&scale==0x10000&&life==0);++debrisCalls;
}
#include "../src/integration/lagi_tutorial_laser_enemy.inc"
struct Probe {unsigned rng=0,deaths=0,hides=0,hits=0,numbers=0,animationBefore=0;std::vector<int> sounds;};
static const uint32_t randomValues[]={0,0xffff,0xffffffff};
static uint32_t random(void* context){auto& p=*static_cast<Probe*>(context);assert(p.rng<3);return randomValues[p.rng++];}
static uint8_t quadrant(void* context){auto& p=*static_cast<Probe*>(context);assert(steps==int(p.animationBefore+1));return 0;}
static bool displayWindow(void*){return false;}
static void onHit(void* context,uint32_t ea,s_workAreaCopy*,sBattleTargetable&){assert(ea==0x0609f440);++static_cast<Probe*>(context)->hits;}
static void hitDisplay(void*,s_workAreaCopy*,sBattleTargetable&,int mode){assert(mode==0);}
static void damageDisplay(void* context,s_workAreaCopy*,int16_t,sVec3_FP*,int mode){assert(mode==1);++static_cast<Probe*>(context)->numbers;}
static void sound(void* context,int value){static_cast<Probe*>(context)->sounds.push_back(value);}
static void death(void* context){++static_cast<Probe*>(context)->deaths;}
static void hide(void* context){++static_cast<Probe*>(context)->hides;}
static int32_t bits(uint32_t v){return v<=0x7fffffffu?int32_t(v):int32_t(int64_t(v)-0x100000000ll);}
static int32_t mul(int32_t a,int32_t b){auto product=int64_t(a)*b;auto q=product/65536;if(product<0&&product%65536)--q;return bits(uint32_t(q));}
#ifndef LAGI_TUTORIAL_ENEMY_FIXTURE_ONLY
int main(){
 s_workAreaCopy owner;owner.header.fill(0xa5);npcFileDeleter resources;
 sVec3_FP pos{},cur{},target{},vel{},acc{},rot{},rotCur{},rotTarget{},rotVel{},rotAcc{},parent{},parentRot{},authored{100,200,300};
 uint32_t flags=0;int16_t hp=100,floor=0,pending=0,color=0,timer=0;uint8_t state=0,meter=0;int8_t deadCount=0;int32_t xp=0,dyne=0;
 std::vector<s_3dModel> models(1);std::vector<s_hotpointDefinition> definitions(1);definitions[0].m4_count=1;definitions[0].m0.resize(1);
 sBattleTargetable hitTarget{};hitTarget.owner=&owner;sVec3_FP hitPosition{};
 auto& model=models[0];model.m12_numBones=model.m50_targetBoneCount=model.m52_targetableCount=1;model.m40=&definitions;
 model.m44_hotpointData={{{9,8,7}}};model.m54_targetables=&hitTarget;model.m58_targetablePositions=&hitPosition;
 LagiBattleModelTargetState targets;targets.hitboxValues.fill(10);targets.damageCallbackEA[0]=0x0609f440;
 LagiBattleModelMotionView motion{pos,cur,target,vel,acc,rot,rotCur,rotTarget,rotVel,rotAcc,parent,parentRot,0xf5c};
 LagiBattleModelDamageView damage{&owner,flags,hp,floor,pending,color,meter,pos,models,targets};
 LagiTutorialLaserEnemyView actor{motion,damage,state,timer,authored,deadCount,xp,dyne,&resources};liveActor=&actor;
 Probe probe;LagiTutorialLaserEnemyHooks hooks{{&probe,quadrant,displayWindow,onHit,hitDisplay,damageDisplay},random,sound,death,hide};
 auto reset=[&](){probe={};probe.animationBefore=steps;owner.task.finished=false;flags=0;hp=100;floor=pending=color=timer=0;meter=deadCount=0;xp=dyne=0;state=0;
  pos=cur=target=vel=acc=rot=rotCur=rotTarget=rotVel=rotAcc=parent=parentRot={};hitTarget.m50_flags=0;hitTarget.m58=0;hitTarget.m5E_impactForce=0;debrisCalls=0;retired=0;};
 // Movement uses the PREVIOUS target before the same update assigns jitter.
 reset();target={0x10000,-0x10000,0x20000};parent={1,2,3};acc={4,5,6};
 lagiUpdateTutorialLaserEnemy(actor,hooks);
 assert(pos.x==0xf5d&&pos.y==-0xf5a&&pos.z==0x1ebb);assert(acc.x==4&&acc.y==5&&acc.z==6);
 assert(probe.rng==3&&target.x==100-32768&&target.y==200+32767&&target.z==300+32767);
 assert(probe.sounds.empty()&&state==0&&probe.deaths==0);
 // A live hit enters and executes state 1 immediately; impulses are applied
 // to angular ACCELERATION, not velocity, and become motion next update.
 reset();hitTarget.m50_flags=0xa0000;hitTarget.m58=12;
 lagiUpdateTutorialLaserEnemy(actor,hooks);
 assert(hp==88&&pending==12&&probe.hits==1&&probe.sounds==std::vector<int>{0x66});
 assert(state==0&&!(flags&0x60000000u)&&probe.rng==3);
 assert(rotAcc.x==-0x200000&&rotAcc.y==-0x1f0001&&rotAcc.z==0x1fffff&&rotVel.x==0);
 probe={};probe.animationBefore=steps;lagiUpdateTutorialLaserEnemy(actor,hooks);
 assert(rotVel.x==-0x200000&&rotAcc.x==0&&rotCur.x==0x0fe00000);
 // Death notification does NOT invent a switch to state 2. Native task/
 // formation ownership chooses that state separately; reward is once-only
 // because the actual retirement gate prevents consuming another hit.
 reset();hp=5;hitTarget.m50_flags=0xa0000;hitTarget.m58=6;xp=0x7ffffffa;dyne=7;
 lagiUpdateTutorialLaserEnemy(actor,hooks);
 assert(hp==-1&&probe.deaths==1&&probe.sounds==std::vector<int>{0x65});
 assert(xp==bits(0x8000000au)&&dyne==17&&state==0&&retired==1&&deadCount==0);
 assert((flags&0x90000000u)==0x90000000u&&probe.numbers==1);
 probe={};probe.animationBefore=steps;hitTarget.m50_flags|=0x80000;
 lagiUpdateTutorialLaserEnemy(actor,hooks);assert(probe.deaths==0&&xp==bits(0x8000000au)&&dyne==17);
 // Authored debris transition, followed by its exact 31+ frame removal gate.
 reset();state=2;lagiUpdateTutorialLaserEnemy(actor,hooks);
 assert(state==3&&probe.hides==1&&debrisCalls==1&&probe.rng==0&&timer==0&&!owner.task.finished);
 size_t timerCases=0;
 for(int initial=-32768;initial<=32767;++initial){
  reset();state=3;timer=int16_t(initial);deadCount=int8_t((initial&255)<128?(initial&255):(initial&255)-256);
  const int oldDead=deadCount;
  lagiUpdateTutorialLaserEnemy(actor,hooks);
  assert(probe.rng==0&&debrisCalls==0&&probe.hides==0);
  if(initial>30){assert(owner.task.finished&&timer==0);const auto wrapped=(oldDead+1)&255;assert(int(deadCount)==(wrapped<128?wrapped:wrapped-256));}
  else {assert(!owner.task.finished&&timer==lagiBattleDamageWord(uint32_t(initial)+1));assert(deadCount==oldDead);}
  ++timerCases;
 }
 // Exercise motion with wrapped signed positions and angles over a trajectory.
 reset();model.m52_targetableCount=0;target={0x7fffffff,-0x7fffffff,0x08000000};cur={-1,65536,-65536};rotTarget={0x08000000,0x0fffffff,-1};
 uint32_t seed=1234;size_t motionCases=0;
 for(int frame=0;frame<512;++frame){
  std::array<int32_t,3> expectedPos{},expectedRot{},expectedVelocity{};
  for(unsigned a=0;a<3;++a){
   seed=seed*1664525u+1013904223u;parent[a].m_value=bits(seed);parentRot[a].m_value=bits(seed^0xa55a55a5u);
   const auto pv=mul(0xf5c,bits(uint32_t(target[a].asS32())-uint32_t(cur[a].asS32())));
   expectedPos[a]=bits(uint32_t(parent[a].asS32())+uint32_t(cur[a].asS32())+uint32_t(pv));
   uint32_t diff=uint32_t(rotTarget[a].asS32())-uint32_t(rotCur[a].asS32());diff=(diff&0x08000000u)?diff|0xf0000000u:diff&0x0fffffffu;
   auto ra=bits(uint32_t(rotAcc[a].asS32())+uint32_t(mul(bits(diff),0xff))-uint32_t(mul(rotVel[a].asS32(),0xfff)));
   expectedVelocity[a]=bits(uint32_t(rotVel[a].asS32())+uint32_t(ra));
   auto next=(uint32_t(rotCur[a].asS32())+uint32_t(expectedVelocity[a]))&0x0fffffffu;
   expectedRot[a]=bits(uint32_t(parentRot[a].asS32())+next);
  }
  probe={};probe.animationBefore=steps;lagiUpdateTutorialLaserEnemy(actor,hooks);
  for(unsigned a=0;a<3;++a){assert(pos[a].asS32()==expectedPos[a]&&rot[a].asS32()==expectedRot[a]&&rotVel[a].asS32()==expectedVelocity[a]&&rotAcc[a].asS32()==0);++motionCases;}
 }
 for(auto b:owner.header)assert(b==0xa5);
 std::printf("PASS: integrated native tutorial enemy update: live/dead hits, motion/animation ordering, debris, %zu signed timer cases and %zu trajectory axes; service mocks, not hardware\n",timerCases,motionCases);
}
#endif
