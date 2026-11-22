#include "lathum_rig_fixture.h"
#include "../include/lagi/native_formation_motion.h"
template<class T>struct s_workAreaTemplate:s_workAreaCopy {struct TypedTaskDefinition{void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
static p_workArea created=nullptr;static bool failAllocation=false;
template<class T>T* createSubTask(p_workArea){if(failAllocation)return nullptr;auto* result=new T;created=result;return result;}
struct sVdp1Quad{};std::vector<sVdp1Quad> quads(1),repairQuads(1);
struct sAnimatedQuad{u16 vram=0;unsigned frame=0;};
int animationSteps=0,quadInitializations=0,draws=0;
void particleInitSub(sAnimatedQuad* q,u16 vram,const std::vector<sVdp1Quad>* data){assert(vram==0x4321&&data==&quads);q->vram=vram;q->frame=0;++quadInitializations;}
int sGunShotTask_UpdateSub4(sAnimatedQuad* q){++q->frame;++animationSteps;return 0;}
int vdp1DrawQuadScaled(sAnimatedQuad*,sVec3_FP*,fixedPoint scale){assert(scale.asS32()==0x30000);++draws;return 0;}
const std::vector<sVdp1Quad>* lagiNativeBattleParticleQuad(sSaturnPtr ea){if(ea.m_offset==0x060A3B9C)return &repairQuads;assert(ea.m_offset==0x060A3AD8);return &quads;}
struct Vram{u16 m4_vdp1Memory=0x4321;}vram;
struct Resource:npcFileDeleter{Vram* m4_vd1Allocation=&vram;}resource;
struct ResourceSlot{Resource* mC_fileBundle=&resource;};ResourceSlot dramAllocatorEnd[32];
struct s_battleEngine{struct{bool m2000=false,m100_attackAnimationFinished=false;}m188_flags;sVec3_FP mC_battleCenter,m1A0_battleAutoScrollDelta,m164;int m22C_dragonCurrentQuadrant=0;};
struct Dragon{sVec3_FP m8_position;sBattleTargetable m8C;u32 m1C0_statusModifiers=0;}dragon;
struct Overlay{Dragon* m18_dragon=&dragon;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;Manager* gBattleManager=&manager;
struct {int m28[2]={11,22};}cameraProperties2;
void transformAndAddVec(const sVec3_FP& in,sVec3_FP& out,int matrix){assert(matrix==22);out=in;out[0]=in[0].asS32()+100;out[1]=in[1].asS32()+200;out[2]=in[2].asS32()+300;}
int divisionCalls=0;
fixedPoint FP_Div(s32 value,fixedPoint divisor){++divisionCalls;assert(divisor.asS32());return s32((int64_t(value)*65536)/divisor.asS32());}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationMultiply(a.asS32(),b.asS32());}
int hits=0,expectedDamage=28;sVec3_FP hitPosition,hitKnockback;
void applyDamageToDragon(sBattleTargetable& target,s32 damage,sVec3_FP& position,s32 kind,const sVec3_FP& knockback,s32 flags){assert(&target==&dragon.m8C&&damage==expectedDamage&&kind==3&&!flags);++hits;hitPosition=position;hitKnockback=knockback;}
unsigned projectileRngCalls=0,firstWord=0,rngTail=0;
u32 projectileRandom(){return projectileRngCalls++==0?firstWord:rngTail;}
#include "damage_word.inc"
#define randomNumber projectileRandom
#include "projectile.inc"
#undef randomNumber
fixedPoint getSin(u32 phase){const auto a=0x16E80+(phase&0xFFF)*4;return s32((u32(common[a])<<24)|(u32(common[a+1])<<16)|(u32(common[a+2])<<8)|common[a+3]);}
fixedPoint getCos(u32 phase){return getSin(phase+1024);}
std::vector<sVec3_FP> ringParticles;
struct RepairParticle{sVec3_FP position,velocity,acceleration;int life;};std::vector<RepairParticle> repairParticles;
struct sParticleTask{sAnimatedQuad m8_quad;};
#define randomNumber projectileRandom
#include "particle_phase.inc"
#undef randomNumber

void createParticleEffect(npcFileDeleter* file,const std::vector<sVdp1Quad>* quad,const sVec3_FP* pos,sVec3_FP* velocity,sVec3_FP* acceleration,s32 scale,const void* colors,s16 life){
 assert(file==static_cast<npcFileDeleter*>(&resource)&&!colors&&scale==0x30000);
 if(quad==&repairQuads){assert(acceleration&&life>=15&&life<=30);repairParticles.push_back({*pos,*velocity,*acceleration,life});}
 else{assert(quad==&quads&&!acceleration&&life==15);for(unsigned axis=0;axis<3;++axis)assert(!(*velocity)[axis].asS32());ringParticles.push_back(*pos);}
 sParticleTask phase;lagiTestParticlePhase(&phase,life);
}
#define randomNumber projectileRandom
#include "ring.inc"
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
std::array<LagiLathumRing,9> capturedRings;int restores=0,interpolations=0;
void battleEngine_restoreCameraDefault(){++restores;}
void battleEngine_resetCameraInterpolation(){++interpolations;}
p_workArea captureRing(p_workArea,LagiLathumRig& rig,u8 group,const sVec3_FP&){firedGroups.push_back(group);capturedRings[group].position={int(group*10),int(group*20),int(group*30)};rig.cameraTarget=capturedRings[group].position;return &capturedRings[group];}
#define lagiCreateLathumRing captureRing
#include "attack_two.inc"
#undef lagiCreateLathumRing
int safetyClears=0;void battleEngine_FlagQuadrantBitForSafety(u16 mask){assert(!mask);++safetyClears;}
#define randomNumber projectileRandom
#include "attack_three.inc"
#undef randomNumber


void check(const sVec3_FP& v,int x,int y,int z){assert(v[0].asS32()==x&&v[1].asS32()==y&&v[2].asS32()==z);}
int main(int argc,char**argv){
 assert(argc==3);original.bytes=load(argv[1]);common=load(argv[2]);s_workAreaCopy owner;LagiLathumRig rig;rig.descriptor=original.getSaturnPtr(0x060A3EB4);rig.corePositions.resize(11);
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

 for(unsigned group=0;group<9;++group)for(unsigned bits=0;bits<64;++bits){
  firstWord=0xFFFF;rngTail=bits;projectileRngCalls=0;divisionCalls=0;hits=0;dragon.m8_position={100000,200000,300000};dragon.m1C0_statusModifiers=0xABCD;
  auto* ring=static_cast<LagiLathumRing*>(lagiCreateLathumRing(&owner,rig,u8(group),dragon.m8_position));const int life=(bits&0x2F)+30;
  assert(ring->remaining==life&&projectileRngCalls==3&&divisionCalls==3&&!ring->getTypedTaskDefinition()->draw);
  check(ring->position,anchors[group]*100+100,anchors[group]*200+200,anchors[group]*300+300);
  auto start=ring->position,oldVelocity=ring->velocity;ringParticles.clear();ring->Update(ring);
  assert(projectileRngCalls==7+(bits&1?4:0)&&ring->angle==0x71C71C&&ringParticles.size()==(bits&1?4:0)&&!ring->task.finished);
  for(unsigned axis=0;axis<3;++axis){int expected=oldVelocity[axis].asS32();if(axis!=1)expected=int((int64_t(0xCCC)*expected)>>16)+int((int64_t(0xCCC)*(dragon.m8_position[axis].asS32()-start[axis].asS32()))>>16);assert(ring->velocity[axis].asS32()==expected&&ring->position[axis].asS32()==start[axis].asS32()+expected);}
  for(unsigned q=0;q<ringParticles.size();++q){const auto phase=(ring->angle+q*0x4000000u)>>16;check(ringParticles[q],ring->position[0].asS32()+int((int64_t(0xA000)*getSin(phase).asS32())>>16),ring->position[1].asS32(),ring->position[2].asS32()+int((int64_t(0xA000)*getCos(phase).asS32())>>16));}
  for(int tick=1;tick<life;++tick){ringParticles.clear();ring->Update(ring);}assert(ring->remaining==0&&!ring->task.finished&&dragon.m1C0_statusModifiers==0xABCD&&!hits);
  ring->Update(ring);assert(ring->remaining==-1&&ring->task.finished&&!dragon.m1C0_statusModifiers&&!hits&&projectileRngCalls==3+(4+(bits&1?4:0))*(life+1));delete ring;
 }
 failAllocation=true;projectileRngCalls=0;assert(!lagiCreateLathumRing(&owner,rig,0,dragon.m8_position)&&!projectileRngCalls);failAllocation=false;
 // Every intact quadrant-facing group fires once, in original group order.
 const unsigned begin[4]={0,3,5,7},end[4]={3,5,7,9};
 for(unsigned state=0;state<19683;++state){unsigned bits=state;for(auto& g:rig.groups){g.state=bits%3;bits/=3;}for(unsigned q=0;q<4;++q){rig.mode=1;rig.substate=1;engine.m22C_dragonCurrentQuadrant=q;firedGroups.clear();lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);unsigned n=0;for(unsigned g=begin[q];g<end[q];++g)if(!rig.groups[g].state){assert(firedGroups.at(n++)==g);}assert(n==firedGroups.size()&&rig.substate==2&&rig.controllerTimer==90);}}
 rig.mode=1;rig.substate=0;engine.m188_flags={};lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(!rig.substate);engine.m188_flags.m2000=true;lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(rig.substate==1&&eye==&dragon.m8_position&&target==&rig.cameraTarget&&enabled==1);
 rig.substate=2;rig.controllerTimer=90;for(int i=0;i<90;++i)lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(rig.controllerTimer==0&&rig.mode==1&&!engine.m188_flags.m100_attackAnimationFinished);lagiUpdateLathumAttackOne(&owner,rig,engine,dragon.m8_position);assert(rig.mode==0&&!rig.substate&&engine.m188_flags.m100_attackAnimationFinished);

 for(unsigned state=0;state<19683;++state){unsigned bits=state;for(auto& g:rig.groups){g.state=bits%3;bits/=3;}for(unsigned q=0;q<4;++q){rig.mode=2;rig.substate=1;rig.ring=nullptr;rig.activeRingCount=9;engine.m22C_dragonCurrentQuadrant=q;firedGroups.clear();lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);unsigned n=0;unsigned last=0;for(unsigned g=begin[q];g<end[q];++g)if(rig.groups[g].state){assert(firedGroups.at(n++)==g);last=g;}assert(n==firedGroups.size()&&rig.activeRingCount==n&&rig.substate==2&&rig.controllerTimer==30);assert(rig.ring==(n?static_cast<p_workArea>(&capturedRings[last]):nullptr));}}
 rig.mode=2;rig.substate=0;engine.m188_flags={};engine.mC_battleCenter={100,200,300};const int oldEnabled=enabled;
 lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);assert(!rig.substate&&enabled==oldEnabled);
 engine.m188_flags.m2000=true;lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);assert(rig.substate==1&&eye==&rig.cameraEye&&target==&rig.cameraTarget&&enabled==oldEnabled+1);check(rig.cameraTarget,100,200,300);check(rig.cameraEye,100,dragon.m8_position[1].asS32(),300);
 for(unsigned q=0;q<4;++q)for(unsigned count=0;count<=3;++count){
  rig.mode=2;rig.substate=2;rig.activeRingCount=u8(count);rig.controllerTimer=1;capturedRings[0].task.finished=false;capturedRings[0].position={111,222,333};rig.ring=&capturedRings[0];engine.m164={};engine.m188_flags={};engine.m22C_dragonCurrentQuadrant=q;hits=0;expectedDamage=count*92;
  lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);assert(rig.substate==2&&rig.controllerTimer==0);check(rig.cameraTarget,111,222,333);
  capturedRings[0].task.finished=true;lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);assert(rig.substate==3&&!rig.ring);
  const int v=count*0x3000,height=int((int64_t(v)*v)/0x2C0)+0x5000;
  check(rig.cameraEye,dragon.m8_position[0].asS32()+(q==1?0x3000:q==3?-0x3000:0),dragon.m8_position[1].asS32()+height,dragon.m8_position[2].asS32()+(q==0?0x3000:q==2?-0x3000:0));check(rig.dragonDisplacement,0,v,0);
  int referenceY=0,referenceVelocity=v,ticks=0;
  while(rig.substate==3){referenceY+=referenceVelocity;referenceVelocity-=0x160;lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);++ticks;assert(ticks<400);assert(engine.m164[1].asS32()==(referenceY>0?referenceY:0));assert(hits==(referenceY>0?0:1));}
  assert(rig.substate==4&&hits==1&&hitKnockback[1].asS32()==referenceVelocity&&hitPosition[0].asS32()==dragon.m8_position[0].asS32());const int oldRestores=restores;lagiUpdateLathumAttackTwo(&owner,rig,engine,dragon.m8_position);assert(!rig.mode&&!rig.substate&&engine.m188_flags.m100_attackAnimationFinished&&restores==oldRestores+1);
 }

 // Full repair cycles: every subset is repaired in ascending group order.
 for(unsigned mask=0;mask<512;++mask){
  rig.mode=3;rig.substate=1;rig.selectedHitCount=17;rig.groupCounter=0;engine.m188_flags={};repairParticles.clear();firstWord=rngTail=0;projectileRngCalls=0;
  for(unsigned g=0;g<9;++g)rig.groups[g].state=(mask&(1u<<g))?(g%2?1:2):0;
  unsigned repaired=0;
  for(unsigned g=0;g<9;++g)if(mask&(1u<<g)){
   const auto before=projectileRngCalls;lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(rig.substate==2&&rig.repairingGroup==g&&rig.controllerTimer==21&&projectileRngCalls==before+128);
   assert(repairParticles.size()==(++repaired)*16);
   const int anchor=anchors[g];for(unsigned particle=(repaired-1)*16;particle<repaired*16;++particle){auto& p=repairParticles[particle];check(p.position,anchor*100+100-32767,anchor*200+200-32767,anchor*300+300-32767);check(p.velocity,-2047,0,engine.m1A0_battleAutoScrollDelta[2].asS32()-2047);check(p.acceleration,0,-88,0);assert(p.life==15);}
   for(int tick=0;tick<21;++tick)lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(rig.substate==2&&rig.groups[g].state);lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(rig.substate==1&&!rig.groups[g].state);
  }
  lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(rig.substate==10);const int oldSafety=safetyClears;lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(!rig.mode&&!rig.substate&&rig.groupCounter==-1&&rig.selectedGroup==0&&rig.selectedHitCount==17&&engine.m188_flags.m100_attackAnimationFinished&&safetyClears==oldSafety+1);
 }
 rig.mode=3;rig.substate=0;rig.orbitAngle=0;engine.m188_flags={};projectileRngCalls=0;firstWord=0x4000000;rngTail=0;const auto beforeAngle=rig.orbitAngle;lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(!rig.substate&&rig.orbitAngle==beforeAngle+0x5B05B&&!projectileRngCalls);
 engine.m188_flags.m2000=true;lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(rig.substate==1&&rig.orbitAngle==0x4000000&&projectileRngCalls==1&&eye==&rig.cameraEye&&target==&rig.cameraTarget);check(rig.cameraEye,engine.mC_battleCenter[0].asS32()+0xFA000,dragon.m8_position[1].asS32()+0x32000,engine.mC_battleCenter[2].asS32());

 for(unsigned value=0;value<18;++value){rig.mode=3;rig.substate=10;firstWord=value;projectileRngCalls=0;lagiUpdateLathumAttackThree(&owner,rig,engine,dragon.m8_position);assert(rig.selectedGroup==value%9&&projectileRngCalls==1);}
 puts("PASS Lathum attacks1-3 and effects: nine anchors, all 32 lifetimes and spring extremes, five visual springs, 28-damage expiry and 78732 group/quadrant states");
}
