#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include "lagi/native_formation_motion.h"
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
struct fixedPoint{s32 value=0;fixedPoint(s32 v=0):value(v){}s32 asS32()const{return value;}};
struct sVec3_FP{fixedPoint x,y,z;fixedPoint& operator[](unsigned i){return i==0?x:i==1?y:z;}const fixedPoint& operator[](unsigned i)const{return i==0?x:i==1?y:z;}};
struct s_workAreaCopy{struct Task{bool finished=false;void markFinished(){finished=true;}}task;Task* getTask(){return &task;}};
template<class T>struct s_workAreaTemplate:s_workAreaCopy{struct TypedTaskDefinition{void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
void* created=nullptr;bool allocationFails=false;
template<class T>T* createSubTask(s_workAreaCopy*){if(allocationFails)return nullptr;auto* p=new T;created=p;return p;}
struct Target{sVec3_FP* m8=nullptr;s32 m4C=0;};
struct Dragon{Target m8C;sVec3_FP m8_position,mF0;}dragon;
struct Engine{sVec3_FP m1A0_battleAutoScrollDelta;}engine;
struct Overlay{Dragon* m18_dragon=&dragon;Engine* m4_battleEngine=&engine;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
struct Model{Target* m54_targetables=nullptr;unsigned m52_targetableCount=0;};
struct sBTL_A5_VariantEnemy:s_workAreaCopy{std::array<Model,1> m2C_models;};
struct Vram{u16 m4_vdp1Memory=0x4321;}vram;
struct Resource{Vram* m4_vd1Allocation=&vram;}resource;
struct Slot{Resource* mC_fileBundle=&resource;};std::array<Slot,15> dramAllocatorEnd;
struct File{u32 getSaturnPtr(u32 a){return a;}}file;auto* g_BTL_A5=&file;
struct sVdp1Quad{};std::vector<sVdp1Quad> quads(1);
const std::vector<sVdp1Quad>* lagiNativeBattleParticleQuad(u32 a){assert(a==0x60AD2D8);return &quads;}
struct sAnimatedQuad{unsigned frame=0;};
unsigned draws=0,steps=0;std::vector<unsigned> drawnFrames;
void particleInitSub(sAnimatedQuad* q,u16 v,const std::vector<sVdp1Quad>* data){assert(v==0x4321&&data==&quads);q->frame=0;}
int drawProjectedParticle(sAnimatedQuad* q,sVec3_FP*){++draws;drawnFrames.push_back(q->frame);return 0;}
int sGunShotTask_UpdateSub4(sAnimatedQuad* q){++steps;++q->frame;return 0;}
unsigned rngCalls=0;u32 timingWord=0;
u32 randomNumber(){const unsigned i=rngCalls++;return i==124?timingWord:i*0x12345+0xABCDEF;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationMultiply(a.asS32(),b.asS32());}
fixedPoint FP_Div(s32 a,fixedPoint b){assert(b.asS32());return s32((int64_t(a)*65536)/b.asS32());}
fixedPoint MTH_Product3d_FP(const sVec3_FP& a,const sVec3_FP& b){int64_t sum=0;for(unsigned i=0;i<3;++i)sum+=int64_t(a[i].asS32())*b[i].asS32();return s32(sum>>16);}
fixedPoint sqrt_F(fixedPoint a){return s32(std::sqrt(double(a.asS32())*65536));}
unsigned hits=0;
void applyDamageToDragon(Target& target,s32 damage,sVec3_FP& position,s32 kind,const sVec3_FP& knockback,s32 flags){assert(&target==&dragon.m8C&&damage==44&&&position==&dragon.m8_position&&kind==2&&flags==0x800);for(unsigned i=0;i<3;++i)assert(!knockback[i].asS32());++hits;}
#include "burst.inc"
int main(){
 for(unsigned timing=0;timing<16;++timing){
  timingWord=timing;rngCalls=hits=draws=steps=0;drawnFrames.clear();dragon.m8C.m4C=0;
  s_workAreaCopy parent;sVec3_FP source={0x10000,-0x20000,0x30000},destination={0x20000,-0x20000,0x30000};engine.m1A0_battleAutoScrollDelta={999,888,777};
  lagiCreateGarilVariantBurst(&parent,source,destination);auto* b=static_cast<LagiGarilVariantBurst*>(created);
  assert(rngCalls==125&&b->hitTimer==((timing&14)+13));
  for(unsigned n=0;n<31;++n){auto& v=b->visuals[n];assert(v.active);for(unsigned axis=0;axis<3;++axis)assert(v.position[axis].asS32()==source[axis].asS32());
   auto random=[&](unsigned i){return (n*4+i)*0x12345u+0xABCDEFu;};
   assert(v.velocity[0].asS32()==0x2800+s32(random(0)&0x1FFF)-0xFFF);
   assert(v.velocity[1].asS32()==s32(random(1)&0x1FFF)-0xFFF);
   assert(v.velocity[2].asS32()==s32(random(2)&0x7FF)-0x3FF+777);
   assert(v.randomWord==(random(3)&0x7F));
  }
  const auto initial=b->visuals[0];
  for(unsigned tick=1;tick<=159;++tick){
   LagiGarilVariantBurst::Update(b);
   assert(steps==0&&rngCalls==125);
   assert(hits==unsigned(b->hitTimer<27&&tick>=unsigned(27-b->hitTimer)));
   assert(b->getTask()->finished==(tick>=159));
   assert(dragon.m8C.m4C==(tick>=159?0x1000:0));
   assert(b->visuals[0].velocity[1].asS32()==initial.velocity[1].asS32()-s32(tick)*0xB0);
  }
  LagiGarilVariantBurst::Draw(b);assert(draws==31&&steps==1);for(auto f:drawnFrames)assert(f==0);
  b->visuals[0].active=false;LagiGarilVariantBurst::Draw(b);assert(draws==61&&steps==2);
  delete b;
 }
 LagiGarilVariantBurst b;b.visuals[0].position[0]=INT32_MAX;b.visuals[0].velocity[0]=1;LagiGarilVariantBurst::Update(&b);assert(b.visuals[0].position[0].asS32()==INT32_MIN);
 sBTL_A5_VariantEnemy actor;std::array<Target,6> targets;std::array<sVec3_FP,6> positions;for(unsigned i=0;i<6;++i){positions[i]={s32(i*10000),0,0};targets[i].m8=&positions[i];}actor.m2C_models[0]={targets.data(),6};dragon.mF0={0x50000,0,0};rngCalls=0;lagiFireGarilVariantBurst(&actor);auto* fired=static_cast<LagiGarilVariantBurst*>(created);assert(fired->visuals[0].position[0].asS32()==50000);delete fired;
 allocationFails=true;rngCalls=0;lagiCreateGarilVariantBurst(&actor,positions[0],dragon.mF0);assert(!rngCalls);
 puts("PASS variant burst: 31 visuals, RNG order, all timing choices, 159-tick expiry, wrap, shared Draw and target 5 launch");
}
