#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;using s32=std::int32_t;
using sVec3_FP=std::array<s32,3>;
struct Task {bool finished=false;void markFinished(){finished=true;}};
struct Work {Task task;Task* getTask(){return &task;}};using p_workArea=Work*;
template<class T,class Arg>struct s_workAreaTemplateWithArg:Work {
 using Fn=void(*)(T*);Fn m_UpdateMethod=nullptr,m_DrawMethod=nullptr;
 struct TypedTaskDefinition {void(*init)(T*,Arg);Fn update,draw,destroy;};
};
static Work* lastCreated=nullptr;
template<class T,class Arg>T* createSiblingTaskWithArg(p_workArea,Arg arg,const typename T::TypedTaskDefinition* definition) {
 auto* result=new T{};result->m_UpdateMethod=definition->update;result->m_DrawMethod=definition->draw;
 lastCreated=result;definition->init(result,arg);return result;
}
#define Unimplemented() ((void)0)
#include "spark_header.inc"
#include "spark_service.inc"
int main(){
 for(unsigned x:{0u,0x4000000u})for(unsigned y:{0u,0x8000000u})for(unsigned lifetime:{1u,20u,255u}) {
  Work parent;sBeamSparkArg arg{};arg.m0_pos={1,2,3};arg.mC=x;arg.m10=y;arg.m14=0x1234;
  arg.m18=lifetime;arg.m1C=0x800;arg.m20=0x14CC;arg.m24=0x4000;arg.m28=-614;arg.m2C=0xFFFF;arg.m2E=0x8421;
  beamSpark_spawn(&parent,&arg);auto* spark=static_cast<sBeamSpark*>(lastCreated);
  assert(spark->m_UpdateMethod==beamSpark_Update);
  assert(spark->m_DrawMethod==((!x&&!y)?beamSpark_UpdateAlt:beamSpark_Draw));
  assert(spark->m0_pos==arg.m0_pos && spark->mC==x && spark->m10==y && spark->m14==arg.m14 && spark->m20==arg.m20);
  assert(spark->m2E==0x421 && !spark->task.finished);
  u32 velocity=arg.m24,radius=arg.m1C;u16 color=arg.m2C;
  for(unsigned frame=1;frame<=lifetime;++frame) {
   velocity+=u32(arg.m28);radius+=velocity;color=u16(color-0x421);
   spark->m_UpdateMethod(spark);
   assert(spark->m24==velocity && spark->m1C==radius && spark->m2C==color && spark->m18==lifetime-frame);
   assert(spark->task.finished==(frame==lifetime));
  }
  assert(spark->m0_pos==arg.m0_pos);delete spark;
 }
 puts("PASS production spark lifecycle: zero/nonzero angle branches preserve update, masked fade/accumulator math and exact 1/20/255-frame retirement; draws tested separately, device pending");
}
