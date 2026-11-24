#include <cassert>
#include <cstdint>
#include <cstdio>
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
constexpr u32 TASK_FLAGS_FINISHED=1;
struct sSaturnPtr{u32 m_offset=0;void* m_file=nullptr;};
struct s_workArea{struct Task{u32 m14_flags=0;void markFinished(){m14_flags|=1;}}task;Task* getTask(){return &task;}};
using p_workArea=s_workArea*;
using s_task=s_workArea::Task;
struct sPCMVoiceTask:s_workArea{int m0=0;}voice;
struct sFormationIntroEffectTask:s_workArea{
 struct TypedTaskDefinition{void(*init)(sFormationIntroEffectTask*,sSaturnPtr);void(*update)(sFormationIntroEffectTask*);void* draw;void(*destroy)(sFormationIntroEffectTask*);};
 const TypedTaskDefinition* definition=nullptr;
 u8 m0_state=0,m1_subState=0;s8 m2_flag=0;u8 m3_entryIndex=0;
 s32 m4_totalFrames=0,m8_currentFrame=0,mC_remainingFrames=0;
 s16 m10_battleStringIndex=0,m12_displayCounter=0;u32 m14_cutsceneFilenameRaw=0;s16 m18_continuationFlag=0;u16 m1A_pad=0;
 sSaturnPtr m1C_dataPtr{};sPCMVoiceTask* m20_cutsceneTask=nullptr;s16 m24_savedCounter=0;u16 m26_pad=0;
 static void Update(sFormationIntroEffectTask*);static void Delete(sFormationIntroEffectTask*){}
};
static bool fail=false,skip=false;static p_workArea expectedParent;static int setupCalls=0,skipCalls=0,loadCalls=0,tableReads=0,clearCalls=0;static s16 nextIndex=11;static int frame=0;
template<class T>T* createSubTaskWithArg(p_workArea parent,sSaturnPtr data,const typename T::TypedTaskDefinition* definition){assert(parent==expectedParent&&!data.m_offset&&!definition->init);if(fail)return nullptr;auto* task=new T;task->definition=definition;return task;}
void formationIntroEffect_HandleSkipKey(sFormationIntroEffectTask* t){++skipCalls;if(skip)t->m0_state=2;}
void formationIntroEffect_ResetFrameCounters(sFormationIntroEffectTask* t){t->m4_totalFrames=0x7FFFFFFF;t->m8_currentFrame=0;t->mC_remainingFrames=0;}
void formationIntroEffect_SetupString(sFormationIntroEffectTask* t){++setupCalls;t->m24_savedCounter=t->m12_displayCounter;t->m2_flag=t->m14_cutsceneFilenameRaw?2:1;if(t->m2_flag==2){t->m20_cutsceneTask=&voice;t->m4_totalFrames=4;}}
void formationIntroEffect_LoadEntry(sFormationIntroEffectTask* t,int index){++loadCalls;assert(t->m1C_dataPtr.m_offset);t->m10_battleStringIndex=index;}
s16 formationIntroEffect_GetEntryStringIndex(sFormationIntroEffectTask* t,int){++tableReads;assert(t->m1C_dataPtr.m_offset);return nextIndex;}
s16 formationIntroEffect_GetEntryFlag(sFormationIntroEffectTask*,int){return -1;}
void formationIntroEffect_TryAdvanceStringMidCutscene(sFormationIntroEffectTask*){assert(false);}
int scriptFunction_605861eSub0Sub0(int){return 2;}int getCutsceneFrameIndex(int){return frame;}
void setupVDP2StringRendering(int a,int b,int c,int d){assert(a==3&&b==0x19&&c==0x26&&d==4);}
void clearVdp2TextArea(){++clearCalls;}
#include "speech_body.inc"
#include "single_speech.inc"
int main(){
 s_workArea parent;expectedParent=&parent;
 auto* task=static_cast<sFormationIntroEffectTask*>(lagiCreateNativeBattleSpeech(&parent,10,0,{0x060A3F64,nullptr}));
 assert(setupCalls==1&&task->m10_battleStringIndex==10&&task->m12_displayCounter==0&&task->m18_continuationFlag==-1&&!task->m1C_dataPtr.m_offset&&task->m20_cutsceneTask==&voice);
 assert(task->definition->destroy==&sFormationIntroEffectTask::Delete);
 for(frame=0;frame<=4;++frame){task->definition->update(task);assert(!task->task.m14_flags);}
 assert(task->m0_state==1&&task->m12_displayCounter==0);task->definition->update(task);assert(task->m0_state==1&&task->m12_displayCounter==-1);task->definition->update(task);assert(task->m0_state==2);task->definition->update(task);
 assert(task->task.m14_flags&&tableReads==0&&loadCalls==0&&skipCalls==8);delete task;
 // The table-driven shared Update retains its original chaining behavior.
 sFormationIntroEffectTask chain;chain.m0_state=2;chain.m1C_dataPtr.m_offset=0x6000000;chain.m18_continuationFlag=-1;
 sFormationIntroEffectTask::Update(&chain);assert(!chain.task.m14_flags&&chain.m3_entryIndex==1&&loadCalls==1&&tableReads==1&&chain.m0_state==0);
 // A skip at state0 must terminate the single task without reading a table.
 task=static_cast<sFormationIntroEffectTask*>(lagiCreateNativeBattleSpeech(&parent,10,0,{}));skip=true;const int reads=tableReads;const int skips=skipCalls;task->definition->update(task);assert(task->task.m14_flags&&tableReads==reads&&skipCalls==skips+1);delete task;skip=false;
 // Native no-voice case retains the signed post-decrement duration gate.
 task=static_cast<sFormationIntroEffectTask*>(lagiCreateNativeBattleSpeech(&parent,3,2,{}));task->definition->update(task);assert(task->m0_state==1);
 for(int i=0;i<3;++i){task->definition->update(task);assert(task->m0_state==1);}task->definition->update(task);assert(task->m0_state==2);task->definition->update(task);assert(task->task.m14_flags);delete task;
 fail=true;const int setups=setupCalls;assert(!lagiCreateNativeBattleSpeech(&parent,10,0,{})&&setups==setupCalls);
 puts("PASS shared single speech: voice/timer/skip completion, owned parameters, no next-table access and retained chaining");
}
