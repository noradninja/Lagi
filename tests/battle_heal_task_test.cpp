#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <new>
#include <cstdio>
#include <algorithm>
using s32=int32_t;using u32=uint32_t;using s16=int16_t;using u16=uint16_t;
using p_workArea=void*;
struct fixedPoint{int value=0;fixedPoint(int n=0):value(n){}int asS32()const{return value;}};
struct sVec3_FP{fixedPoint v[3];fixedPoint& operator[](unsigned i){return v[i];}const fixedPoint& operator[](unsigned i)const{return v[i];}};
struct sSaturnPtr{u32 address=0;};
struct Control{bool finished=false;void markFinished(){finished=true;}};
template<class T>struct s_workAreaTemplateWithCopy{
 struct TypedTaskDefinition{void* init;void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};
 void(*m_UpdateMethod)(T*)=nullptr;void(*m_DrawMethod)(T*)=nullptr;Control control;
 Control* getTask(){return &control;}
};
void* created;
template<class T,class P>T* createSubTaskWithCopy(P*,const typename T::TypedTaskDefinition* def){auto* p=new T;p->m_UpdateMethod=def->update;p->m_DrawMethod=def->draw;created=p;return p;}
std::vector<void*> allocations;
void* allocateHeapForTask(void*,size_t bytes){auto* p=::operator new(bytes);allocations.push_back(p);return p;}
struct sVdp1Quad{};
struct sAnimatedQuad{const std::vector<sVdp1Quad>* data{};u16 vram{};};
struct s_battleEngine{struct{int m100_attackAnimationFinished=0;}m188_flags;};
struct Overlay{s_battleEngine* m4_battleEngine;};struct Manager{Overlay* m10_battleOverlay;};
s_battleEngine engine;Overlay overlay{&engine};Manager manager{&overlay};Manager* gBattleManager=&manager;
struct Data{sSaturnPtr getSaturnPtr(u32 a){return {a};}}data;Data* g_BTL_GenericData=&data;
sSaturnPtr readSaturnEA(sSaturnPtr p){assert(p.address==0x060ABF1C);return {0x060AC000};}
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr p){assert(p.address==0x060AC000);return std::vector<sVdp1Quad>(1);}
sVec3_FP readSaturnVec3(sSaturnPtr p){assert(p.address==0x060AD4C8);return {};}
struct Allocation{u16 m4_vdp1Memory=0x100;};Allocation allocation;
struct Bundle{Allocation* m4_vd1Allocation=&allocation;};Bundle bundle;
struct Entry{Bundle* mC_fileBundle=&bundle;};Entry dramAllocatorEnd[1];
unsigned rngCalls=0,quadInits=0,quadUpdates=0,emissions=0,fadeCalls=0;
u32 rng=1;u32 randomNumber(){++rngCalls;rng=rng*1664525u+1013904223u;return rng;}
fixedPoint getSin(u32 a){assert(a<4096);return a<2048?65536:-65536;}
fixedPoint getCos(u32 a){assert(a<4096);return a<1024||a>=3072?65536:-65536;}
void particleInitSub(sAnimatedQuad* p,u16 vram,const std::vector<sVdp1Quad>* q){++quadInits;p->data=q;p->vram=vram;}
bool animationEnd=false;
int sGunShotTask_UpdateSub4(sAnimatedQuad* p){assert(p->data&&p->vram==0x100);++quadUpdates;return animationEnd?3:0;}
int matrixDepth=0;
void pushCurrentMatrix(){++matrixDepth;}void popMatrix(){assert(matrixDepth>0);--matrixDepth;}
void translateCurrentMatrix(const sVec3_FP&){}
void rotateCurrentMatrixYXZ(const sVec3_FP&){}
#include "lagi_battle_heal_effect.inc"
int lagiDrawScaledParticleWithColorTable(sAnimatedQuad* p,sVec3_FP*,fixedPoint scale,sSaturnPtr color){assert(p->data&&scale.asS32()==0x8000&&color.address==0x060AB2BC);++emissions;return 1;}
void urchinFormation_createFadeTask(p_workArea,u16 a,u16 b,u16 c,u16 d,u16 e,s16 f){assert(a==0xC210&&b==0xE739&&c==30&&d==b&&e==a&&f==10);++fadeCalls;}
int main(){
 sVec3_FP dragon{};
 createHealVisualEffect(&engine,{0x060AB2BC},&dragon,0x1800,4,40,0x8000,1);
 auto* task=static_cast<sHealVisualEffectTask*>(created);
 assert(task&&rngCalls==40*(10*4+7)&&quadInits==40);
 unsigned maximumDelay=0;
 for(int i=0;i<40;++i){assert(task->particles[i].seed.type<3);maximumDelay=std::max<unsigned>(maximumDelay,task->particles[i].seed.delay);}
 unsigned at69=0;
 for(unsigned frame=1;frame<=maximumDelay+101;++frame){
  sHealVisualEffectTask_Update(task);
  if(frame==69)at69=rngCalls;
  if(frame>=70)assert(rngCalls==at69);
  sHealVisualEffectTask_Draw(task);
  assert(matrixDepth==0);
  assert(task->control.finished==(frame==maximumDelay+101));
  if(frame<60)assert(engine.m188_flags.m100_attackAnimationFinished==0);
  else assert(engine.m188_flags.m100_attackAnimationFinished==1);
  if(frame==70)for(int i=0;i<40;++i)assert(task->particles[i].seed.state==1);
 }
 assert(fadeCalls==1&&quadUpdates>0&&emissions>0);
 sHealVisualEffectTask_Delete(task);assert(task->quads==nullptr);
 delete task;for(auto p:allocations)::operator delete(p);
 puts("PASS: production heal task allocation/RNG, frame gates, delayed particles, quad updates, draw ownership, completion and cleanup. Not Vita validation.");
 return 0;
}
