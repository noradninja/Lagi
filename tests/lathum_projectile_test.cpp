#include "lathum_rig_fixture.h"
#include "../include/lagi/native_formation_motion.h"
template<class T>struct s_workAreaTemplate:s_workAreaCopy {struct TypedTaskDefinition{void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
static p_workArea created=nullptr;static bool failAllocation=false;
template<class T>T* createSubTask(p_workArea){if(failAllocation)return nullptr;auto* result=new T;created=result;return result;}
struct sVdp1Quad{};std::vector<sVdp1Quad> quads(1);
struct sAnimatedQuad{u16 vram=0;unsigned frame=0;};
int animationSteps=0,quadInitializations=0,draws=0;
void particleInitSub(sAnimatedQuad* q,u16 vram,const std::vector<sVdp1Quad>* data){assert(vram==0x4321&&data==&quads);q->vram=vram;q->frame=0;++quadInitializations;}
int sGunShotTask_UpdateSub4(sAnimatedQuad* q){++q->frame;++animationSteps;return 0;}
int vdp1DrawQuadScaled(sAnimatedQuad*,sVec3_FP*,fixedPoint scale){assert(scale.asS32()==0x30000);++draws;return 0;}
const std::vector<sVdp1Quad>* lagiNativeBattleParticleQuad(sSaturnPtr ea){assert(ea.m_offset==0x060A3AD8);return &quads;}
struct Vram{u16 m4_vdp1Memory=0x4321;}vram;
struct Resource{Vram* m4_vd1Allocation=&vram;}resource;
struct ResourceSlot{Resource* mC_fileBundle=&resource;};ResourceSlot dramAllocatorEnd[32];
struct s_battleEngine{struct{bool m2000=false,m100_attackAnimationFinished=false;}m188_flags;sVec3_FP mC_battleCenter,m1A0_battleAutoScrollDelta;int m22C_dragonCurrentQuadrant=0;};
struct Dragon{sVec3_FP m8_position;sBattleTargetable m8C;}dragon;
struct Overlay{Dragon* m18_dragon=&dragon;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;Manager* gBattleManager=&manager;
struct {int m28[2]={11,22};}cameraProperties2;
void transformAndAddVec(const sVec3_FP& in,sVec3_FP& out,int matrix){assert(matrix==22);out=in;out[0]=in[0].asS32()+100;out[1]=in[1].asS32()+200;out[2]=in[2].asS32()+300;}
int divisionCalls=0;
fixedPoint FP_Div(s32 value,fixedPoint divisor){++divisionCalls;assert(divisor.asS32());return s32((int64_t(value)*65536)/divisor.asS32());}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationMultiply(a.asS32(),b.asS32());}
int hits=0;sVec3_FP hitPosition,hitKnockback;
void applyDamageToDragon(sBattleTargetable& target,s32 damage,sVec3_FP& position,s32 kind,const sVec3_FP& knockback,s32 flags){assert(&target==&dragon.m8C&&damage==28&&kind==3&&!flags);++hits;hitPosition=position;hitKnockback=knockback;}
unsigned projectileRngCalls=0,firstWord=0,rngTail=0;
u32 projectileRandom(){return projectileRngCalls++==0?firstWord:rngTail;}
#include "damage_word.inc"
#define randomNumber projectileRandom
#include "projectile.inc"
#undef randomNumber
std::vector<unsigned> firedGroups;sVec3_FP* eye=nullptr; sVec3_FP* target=nullptr;int enabled=0,sounds=0;
void battleEngine_setCurrentCameraPositionPointer(sVec3_FP* p){eye=p;}
void battleEngine_setDesiredCameraPositionPointer(sVec3_FP* p){target=p;}
void battleEngine_enableAttackCamera(){++enabled;}
void playSystemSoundEffect(int sound){assert(sound==0x6D);++sounds;}
void captureCreate(p_workArea,LagiLathumRig&,u8 group,s_battleEngine&,const sVec3_FP&){firedGroups.push_back(group);}
#define lagiCreateLathumProjectile captureCreate
#include "attack_one.inc"
#undef lagiCreateLathumProjectile
void check(const sVec3_FP& v,int x,int y,int z){assert(v[0].asS32()==x&&v[1].asS32()==y&&v[2].asS32()==z);}
int main(int argc,char**argv){
 assert(argc==2);original.bytes=load(argv[1]);s_workAreaCopy owner;LagiLathumRig rig;rig.descriptor=original.getSaturnPtr(0x060A3EB4);rig.corePositions.resize(11);
 for(int i=0;i<11;++i)rig.corePositions[i]={i*100,i*200,i*300};s_battleEngine engine;engine.m1A0_battleAutoScrollDelta={10,-20,30};dragon.m8_position={100000,200000,300000};
 const int anchors[9]={8,3,9,10,6,0,4,5,7};
 for(unsigned group=0;group<9;++group)for(unsigned seed=0;seed<32;++seed)for(unsigned tail:{0u,8191u}){
  rngTail=tail;
  firstWord=seed;projectileRngCalls=0;divisionCalls=0;animationSteps=quadInitializations=hits=draws=0;
  lagiCreateLathumProjectile(&owner,rig,u8(group),engine,dragon.m8_position);auto* shot=static_cast<LagiLathumProjectile*>(created);const int life=75+seed;
  assert(shot->remaining==life&&projectileRngCalls==16&&divisionCalls==3&&quadInitializations==5&&animationSteps==10);
  check(shot->position,anchors[group]*100+100,anchors[group]*200+200+0xA000,anchors[group]*300+300);
  check(rig.cameraTarget,shot->position[0].asS32(),shot->position[1].asS32(),shot->position[2].asS32());
  for(unsigned axis=0;axis<3;++axis){const int delta=dragon.m8_position[axis].asS32()-shot->position[axis].asS32();assert(shot->velocity[axis].asS32()==int((int64_t(65536/life)*delta)>>16)+engine.m1A0_battleAutoScrollDelta[axis].asS32());}
  for(unsigned i=0;i<5;++i){assert(shot->visuals[i].quad.frame==i);const int jitter=int(tail)-4095;check(shot->visuals[i].velocity,jitter,jitter,jitter);}
  const auto initial=shot->position,velocity=shot->velocity;
  shot->Draw(shot);assert(draws==5);
  shot->Update(shot);assert(!hits&&shot->remaining==life-1&&!shot->task.finished&&projectileRngCalls==31);
  for(unsigned axis=0;axis<3;++axis){const int core=initial[axis].asS32()+velocity[axis].asS32();const int spring=int((int64_t(655+(tail&0xFFF))*velocity[axis].asS32())>>16);assert(shot->position[axis].asS32()==core);for(auto& visual:shot->visuals){assert(visual.velocity[axis].asS32()==int(tail)-4095+spring&&visual.position[axis].asS32()==initial[axis].asS32()+int(tail)-4095+spring);}}
  for(int i=1;i<life;++i)shot->Update(shot);assert(!hits&&shot->remaining==0&&!shot->task.finished);
  dragon.m8_position={-10000,50000,-30000};shot->Update(shot);assert(hits==1&&shot->remaining==-1&&shot->task.finished&&projectileRngCalls==16+15*(life+1));
  for(unsigned axis=0;axis<3;++axis){assert(hitPosition[axis].asS32()==initial[axis].asS32()+velocity[axis].asS32()*(life+1));assert(hitKnockback[axis].asS32()==dragon.m8_position[axis].asS32()-hitPosition[axis].asS32());}
  delete shot;dragon.m8_position={100000,200000,300000};
 }
 failAllocation=true;projectileRngCalls=0;lagiCreateLathumProjectile(&owner,rig,0,engine,dragon.m8_position);assert(!projectileRngCalls);failAllocation=false;
 // Every intact quadrant-facing group fires once, in original group order.
 const unsigned begin[4]={0,3,5,7},end[4]={3,5,7,9};
 for(unsigned state=0;state<19683;++state){unsigned bits=state;for(auto& g:rig.groups){g.state=bits%3;bits/=3;}for(unsigned q=0;q<4;++q){rig.mode=1;rig.substate=1;engine.m22C_dragonCurrentQuadrant=q;firedGroups.clear();lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);unsigned n=0;for(unsigned g=begin[q];g<end[q];++g)if(!rig.groups[g].state){assert(firedGroups.at(n++)==g);}assert(n==firedGroups.size()&&rig.substate==2&&rig.controllerTimer==90);}}
 rig.mode=1;rig.substate=0;engine.m188_flags={};lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(!rig.substate);engine.m188_flags.m2000=true;lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(rig.substate==1&&eye==&dragon.m8_position&&target==&rig.cameraTarget&&enabled==1);
 rig.substate=2;rig.controllerTimer=90;for(int i=0;i<90;++i)lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(rig.controllerTimer==0&&rig.mode==1&&!engine.m188_flags.m100_attackAnimationFinished);lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(rig.mode==0&&!rig.substate&&engine.m188_flags.m100_attackAnimationFinished);
 puts("PASS Lathum timed projectile/attack1: nine anchors, all 32 lifetimes and spring extremes, five visual springs, 28-damage expiry and 78732 group/quadrant states");
}
