#include <cassert>
#include <cstdio>
using s32=int;
struct Task {bool finished=false;void markFinished(){finished=true;}};
template<class T>struct s_workAreaTemplate {Task task;Task* getTask(){return &task;}};
struct Channel {int m0_color=42,m20_stopped=0;};
struct {Channel m0_fade0,m24_fade1;int m_4D=5,m_4C=0;}g_fadeControls;
struct {struct {int m112_CLOFSL=0;}m20_registers[2];}vdp2Controls;
struct {int m28_status=0;void* m8_pSubFieldData=nullptr;}field,*fieldTaskPtr=&field;
int calls=0,prepared=0,deleted=0;
int convertColorToU32ForFade(int c){return c;}
void fadePalette(Channel* c,int from,int to,int steps){assert(from==42&&to==0x8000&&steps==20);++calls;c->m20_stopped=0;}
void exitCutsceneTaskUpdateSub0Sub1Sub0Sub0(int index){assert(index==2);++prepared;}
void exitCutsceneTaskUpdateSub0Sub1Sub0Sub1(int index,int param,int exit){assert(index==2&&param==5&&exit==1);++deleted;}
#include "field_exit_task.inc"
int main(){
 exitCutsceneTaskUpdateSub0Sub1Sub0 task{};
 task.m0=0x8000;task.m4_fieldIndex=2;task.m8=5;task.mC_exitNumber=1;task.m10=20;
 task.update(&task);assert(calls==2&&task.m14_status==1&&(field.m28_status&0x100));
 assert(vdp2Controls.m20_registers[0].m112_CLOFSL==16&&vdp2Controls.m20_registers[1].m112_CLOFSL==16);
 for(int i=0;i<20;++i)task.update(&task);
 assert(calls==2&&!deleted&&!prepared); // no restart or early resource teardown
 g_fadeControls.m0_fade0.m20_stopped=1;task.update(&task);
 assert(prepared==1&&deleted==1&&task.m14_status==2);
 task.update(&task);assert(task.m14_status==2&&!task.task.finished);
 field.m8_pSubFieldData=&task;task.update(&task);assert(task.m14_status==3);
 task.update(&task);assert(task.task.finished&&!(field.m28_status&0x100));
 puts("PASS generated shared field exit: dual native fade, primary completion barrier, single teardown and replacement-field wait; not Vita presentation proof");
}
