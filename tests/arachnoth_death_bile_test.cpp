#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int;using u32=std::uint32_t;
struct sVec3_FP:std::array<int,3>{void zeroize(){fill(0);}};
struct sSaturnPtr{int value;};
struct Overlay{sSaturnPtr getSaturnPtr(int ea){assert(ea==0x060A9624);return {ea};}} overlay,*g_BTL_A3_2=&overlay;
int readSaturnS32(sSaturnPtr p){assert(p.value==0x060A9624);return 8;}
struct Allocation{unsigned short m4_vdp1Memory=0x3456;} allocation;
struct s_workAreaCopy{Allocation* m4_vd1Allocation=&allocation;};
struct {s_workAreaCopy* mC_fileBundle;}dramAllocatorEnd[9];
struct sTrailRenderer{sVec3_FP* m0_positions=nullptr;};
struct sBileProjectile:s_workAreaCopy{
 struct TypedTaskDefinition{void(*init)(sBileProjectile*);void(*update)(sBileProjectile*);void(*draw)(sBileProjectile*);void(*destroy)(sBileProjectile*);};
 sVec3_FP m8_acceleration{},m14_velocity{},m20_position{};sTrailRenderer m2C_trail;int m44_lifetime{};
};
void bileProjectile_update(sBileProjectile*){}void bileProjectile_draw(sBileProjectile*){}
int gouraudDraws=0;void lagiDrawBattleTrailGouraud(sTrailRenderer*){++gouraudDraws;}
const sBileProjectile::TypedTaskDefinition* definition=nullptr;sBileProjectile* task=nullptr;
template<class T>T* createSubTaskWithCopy(s_workAreaCopy*,const typename T::TypedTaskDefinition* d){definition=d;return task=new T;}
int randomCalls=0;int randomNumber(){++randomCalls;return randomCalls%2?0x12340000:0xABCDEF01;}
int MTH_Mul(int a,int b){return int((std::int64_t(a)*b)>>16);}
int MTH_Mul_5_6(int a,int b,int c){return MTH_Mul(MTH_Mul(a,b),c);}
int getSin(int a){assert(a>=0&&a<=0xFFF);return a*8;}
int getCos(int a){assert(a>=0&&a<=0xFFF);return 0x10000-a*8;}
void trailRenderer_init(sTrailRenderer* t,sBileProjectile* owner,sVec3_FP* p,unsigned short base,sSaturnPtr data){
 assert(owner==task&&base==0x3456&&data.value==0x060A9624&&*p==task->m20_position);t->m0_positions=new sVec3_FP[2];
}
#include "../src/integration/lagi_arachnoth_death_bile.inc"
int main(){
 s_workAreaCopy owner;dramAllocatorEnd[8].mC_fileBundle=&owner;sVec3_FP p;p[0]=7;p[1]=8;p[2]=9;
 lagiCreateArachnothBile(&owner,&p,0x1000,1);
 assert(randomCalls==2&&task->m44_lifetime==120&&task->m20_position==p&&task->m8_acceleration==sVec3_FP{});
 const int pitch=MTH_Mul(0x1234,0x08000000)-0x08000000;const int yaw=0xABCDEF01&0x0FFFFFFF;
 assert(task->m14_velocity[0]==MTH_Mul_5_6(getCos((pitch>>16)&4095),getSin((yaw>>16)&4095),4096));
 assert(task->m14_velocity[1]==MTH_Mul(-getSin((pitch>>16)&4095),4096));
 assert(task->m14_velocity[2]==MTH_Mul_5_6(getCos((pitch>>16)&4095),getCos((yaw>>16)&4095),4096));
 definition->draw(task);assert(gouraudDraws==1);definition->destroy(task);assert(!task->m2C_trail.m0_positions);delete task;
 puts("PASS production death bile factory: descriptor owner, native random cone, lifetime, Gouraud callback and cleanup");
}
