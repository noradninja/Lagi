#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include "lagi/native_formation_motion.h"
using s16=int16_t;using u16=uint16_t;using s32=int32_t;
struct fixedPoint {s32 value=0;fixedPoint(s32 v=0):value(v){}s32 asS32()const{return value;}};
struct sVec3_FP {std::array<fixedPoint,3> values;fixedPoint& operator[](unsigned i){return values[i];}const fixedPoint& operator[](unsigned i)const{return values[i];}};
struct s_workArea {struct Task{bool finished=false;void markFinished(){finished=true;}} task;Task* getTask(){return &task;}};
template<class T>struct s_workAreaTemplate:s_workArea{struct TypedTaskDefinition{void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
static s_workArea* expectedParent;static bool fail;static int divides;
template<class T>T* createSubTask(s_workArea* parent){assert(parent==expectedParent);return fail?nullptr:new T;}
fixedPoint FP_Div(s32 dividend,fixedPoint divisor){++divides;return divisor.value?fixedPoint(s32((int64_t(dividend)*65536)/divisor.value)):fixedPoint(0x7fffffff);}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationSignedBits(uint32_t(uint64_t(int64_t(a.value)*b.value)>>16));}
#include "../src/integration/lagi_native_vector_transition.inc"
int main(){
 s_workArea parent;expectedParent=&parent;
 // Native emergence duration 180: reciprocal 364, not exact division by180.
 sVec3_FP live{{1000,-1000,50}},end{{100000,-100000,50}};divides=0;
 auto* t=static_cast<LagiNativeVectorTransition*>(lagiCreateNativeVectorTransition(&parent,&live,end,180));
 assert(divides==3&&t->remaining==180&&t->live==&live);
 assert(t->velocity[0].value==549&&t->velocity[1].value==-550&&t->velocity[2].value==0);
 end[0]=999999; // The original endpoint was copied into velocity at creation.
 for(int frame=1;frame<=180;++frame){t->getTypedTaskDefinition()->update(t);assert(t->task.finished==(frame==180));}
 assert(live[0].value==99820&&live[1].value==-100000&&live[2].value==50);delete t;
 // A second child can move a second live vector without sharing task state.
 sVec3_FP a{{0,0,0}},b{{0,0,0}},aEnd{{65536,65536,65536}},bEnd{{-65536,-65536,-65536}};
 auto* first=static_cast<LagiNativeVectorTransition*>(lagiCreateNativeVectorTransition(&parent,&a,aEnd,270));
 auto* second=static_cast<LagiNativeVectorTransition*>(lagiCreateNativeVectorTransition(&parent,&b,bEnd,180));
 assert(first->velocity[0].value==242&&second->velocity[0].value==-364);
 for(int i=0;i<180;++i){first->Update(first);second->Update(second);}assert(!first->task.finished&&second->task.finished&&a[0].value==43560&&b[0].value==-65520);delete first;delete second;
 // Signed word timer wrap is significant even for non-authored edge inputs.
 for(s16 duration: {s16(1),s16(0),s16(-1),s16(-32768),s16(32767)}){
  sVec3_FP v{{0,0,0}},endpoint{{0,0,0}};auto* task=static_cast<LagiNativeVectorTransition*>(lagiCreateNativeVectorTransition(&parent,&v,endpoint,duration));
  const int ticks=duration==-32768?32768:duration>0?duration:1;
  for(int i=1;i<=ticks;++i){task->Update(task);assert(task->task.finished==(i==ticks));}delete task;
 }
 // Native adds wrap at32 bits, independent of the host's signed-overflow rules.
 sVec3_FP wrap{{INT32_MAX,INT32_MIN,0}},goal{{INT32_MIN,INT32_MAX,0}};
 auto* edge=static_cast<LagiNativeVectorTransition*>(lagiCreateNativeVectorTransition(&parent,&wrap,goal,1));
 assert(edge->velocity[0].value==1&&edge->velocity[1].value==-1);edge->Update(edge);assert(wrap[0].value==INT32_MIN&&wrap[1].value==INT32_MAX);delete edge;
 fail=true;divides=0;assert(!lagiCreateNativeVectorTransition(&parent,&wrap,goal,180)&&!divides);
 puts("PASS native vector transition: authored durations, reciprocal remainder, live ownership, task completion and wrap");
}
