#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct Task {bool finished=false;void markFinished(){finished=true;}};
struct s_workArea {virtual ~s_workArea()=default;Task task;Task* getTask(){return &task;}};
template<class T,class Arg>struct s_workAreaTemplateWithArg:s_workArea {
 struct TypedTaskDefinition {void(*init)(T*,Arg);void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};
};
template<class T,class Arg>T* createSubTaskWithArg(s_workArea*,Arg arg){auto* task=new T;T::Init(task,arg);return task;}
struct {unsigned m14_numVsyncPerFrame=2;}vblankData;
struct {struct {struct {struct {unsigned m8_newButtonDown=0;}m0_current;}m0_inputDevices[1];}m4514;}graphicEngineStatus;
struct Channel {int marker;};
struct {Channel m0_fade0{0},m24_fade1{1};}g_fadeControls;
struct Scroll {int layer,x,y,count;};
std::vector<Scroll> scrolls;
std::vector<int> sounds;
std::vector<std::array<int,2>> highlights;
bool setupOk=true,loadOk=true,cursorOk=true,confirmationOk=true;
int setups=0,clears=0,covers=0,loads=0,restores=0,commits=0,fades=0;
int prompt=-1;bool setupStandalone=false;
LagiAzelMapViewSource lastView;
std::shared_ptr<LagiAzelMapConfirmationState> lastConfirmation;
s_workArea child;
bool lagiAzelSetupMapEntry(bool standalone){++setups;setupStandalone=standalone;return setupOk;}
void startVdp2LayerScroll(int layer,int x,int y,int count){scrolls.push_back({layer,x,y,count});}
bool lagiAzelHighlightMapSelection(int slot,bool selected){highlights.push_back({slot,int(selected)});return true;}
s_workArea* lagiAzelCreateMapWorldUnavailableCover(s_workArea*){++covers;return &child;}
s_workArea* lagiAzelCreateMapSelectionCursor(s_workArea*,std::shared_ptr<LagiAzelMapCursorState> state){state->ready=cursorOk;return cursorOk?&child:nullptr;}
bool lagiAzelPresentMapMessage(int selected,const std::array<std::int32_t,2>&){prompt=selected;return true;}
void lagiAzelClearMapMessage(){prompt=-1;}
s_workArea* lagiAzelCreateFieldMapView(s_workArea*,const LagiAzelMapViewSource& source){lastView=source;return source.resourcesReady?&child:nullptr;}
s_workArea* lagiAzelCreateMapWorldConfirmation(s_workArea*,std::shared_ptr<LagiAzelMapConfirmationState> state){lastConfirmation=state;state->running=confirmationOk;return confirmationOk?&child:nullptr;}
bool lagiAzelCommitMapWorldConfirmation(const LagiAzelMapConfirmationState& state){if(!state.finished || state.running || !state.result)return false;++commits;return true;}
void playSystemSoundEffect(int sound){sounds.push_back(sound);}
void setupVDP2StringRendering(int x,int y,int w,int h){assert(x==0 && y==0x22 && w==0x2C && h==0x1C);}
void clearVdp2TextArea(){++clears;}
void fadePalette(Channel* channel,int from,int to,int duration){assert(channel==&g_fadeControls.m0_fade0 || channel==&g_fadeControls.m24_fade1);assert(from==0 && to==0 && duration==1);++fades;}
bool LagiAzelMapResources::load(const LagiAzelMapDescriptor&){++loads;active_=true;ready_=loadOk;patterns_.fill(0xAB);return ready_;}
void LagiAzelMapResources::restore(){if(active_){++restores;active_=false;}ready_=false;}
LagiAzelMapResources::~LagiAzelMapResources(){restore();}
#include "map_parent.inc"
static void draw(LagiMapParentTask& task,unsigned input=0){graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown=input;LagiMapParentTask::Draw(&task);}
static void destroy(LagiMapParentTask* task){LagiMapParentTask::Delete(task);delete task;}
static void open(LagiMapParentTask& task){for(int i=0;i<16;++i){draw(task);assert(task.cursor->selected==-1);}assert(task.lifecycle.phase==1);draw(task);assert(task.lifecycle.phase==2 && task.cursor->selected==task.selected);}
int main(){
 s_workArea parent;
 LagiAzelMapParentSource source;
 source.hasContext=true;source.context.field=2;source.context.descriptor.nativeAddress=0x20E900;
 source.availability={{1,1},0,false};
 auto* task=static_cast<LagiMapParentTask*>(lagiAzelCreateMapParent(&parent,source));
 assert(task && !setupStandalone && task->cursor->selected==-1 && loads==1 && covers==0);
 assert(scrolls.size()==2 && scrolls[0].layer==0 && scrolls[0].x==-10 && scrolls[0].count==16);
 assert(scrolls[1].layer==1 && scrolls[1].y==-16 && scrolls[1].count==16);
 source.context.field=99;assert(task->source.context.field==2);
 open(*task);assert(prompt==0);
 draw(*task,2);assert(task->lifecycle.mode==1 && task->cursor->selected==-1 && prompt==-1);
 assert(lastView.context.field==2 && lastView.resourcesReady && lastView.patterns[0]==0xAB);
 draw(*task);assert(task->lifecycle.mode==1 && restores==0);
 lastView.status->finished=true;draw(*task);assert(task->lifecycle.mode==0 && prompt==0 && task->cursor->selected==0);
 draw(*task,0x20);assert(task->selected==1 && prompt==1 && sounds.back()==10);
 draw(*task,2);assert(task->lifecycle.mode==2 && task->cursor->selected==-1);
 draw(*task);assert(task->lifecycle.mode==2 && commits==0);
 lastConfirmation->running=false;lastConfirmation->finished=true;lastConfirmation->result=1;
 draw(*task);assert(task->lifecycle.mode==0 && commits==1 && prompt==1);
 draw(*task);assert(commits==1);
 draw(*task,2);assert(task->lifecycle.mode==2);
 lastConfirmation->running=false;lastConfirmation->finished=true;lastConfirmation->result=0;
 draw(*task);assert(task->lifecycle.mode==0 && commits==1); // Native NO result.
 confirmationOk=false;
 draw(*task,2);assert(task->confirmation->finished && task->lifecycle.mode==2);
 draw(*task);assert(task->lifecycle.mode==0 && commits==1);
 confirmationOk=true;
 draw(*task,3);assert(task->lifecycle.phase==3 && sounds.back()==1); // B precedes confirm.
 draw(*task);assert(task->lifecycle.phase==4 && task->lifecycle.countdown==15 && clears==1);
 assert(scrolls[2].x==10 && scrolls[3].y==16);
 for(int i=0;i<14;++i){draw(*task);assert(!task->getTask()->finished);}
 draw(*task);assert(task->getTask()->finished && restores==0);
 destroy(task);assert(restores==1 && vblankData.m14_numVsyncPerFrame==2);

 source.context.field=2;source.standalone=true;
 task=static_cast<LagiMapParentTask*>(lagiAzelCreateMapParent(&parent,source));
 assert(task && setupStandalone && vblankData.m14_numVsyncPerFrame==1);
 draw(*task);assert(task->lifecycle.mode==1 && task->cursor->selected==-1);
 lastView.status->finished=true;draw(*task);assert(task->lifecycle.phase==3 && !task->getTask()->finished);
 draw(*task);assert(task->getTask()->finished && fades==2);
 destroy(task);assert(vblankData.m14_numVsyncPerFrame==2);

 source.standalone=false;source.hasContext=false;source.availability={{0,-1},0,true};
 task=static_cast<LagiMapParentTask*>(lagiAzelCreateMapParent(&parent,source));open(*task);
 assert(covers==1);draw(*task,2);assert(sounds.back()==5 && task->lifecycle.mode==0);
 draw(*task,0x20);assert(task->selected==0);destroy(task);
 assert(!lagiAzelCreateMapParent(nullptr,source));
 source.availability.selected=1;assert(!lagiAzelCreateMapParent(&parent,source));
 source.availability={{1,-1},0,true};assert(!lagiAzelCreateMapParent(&parent,source));
 source.hasContext=true;source.availability={{0,-1},0,true};assert(!lagiAzelCreateMapParent(&parent,source));
 source.hasContext=true;source.availability={{1,1},0,false};loadOk=false;
 task=static_cast<LagiMapParentTask*>(lagiAzelCreateMapParent(&parent,source));open(*task);
 draw(*task,2);assert(task->view->finished);draw(*task);assert(task->lifecycle.mode==0);destroy(task);
 loadOk=true;setupOk=false;source.standalone=true;
 task=static_cast<LagiMapParentTask*>(lagiAzelCreateMapParent(&parent,source));
 assert(task->getTask()->finished && !task->cadenceSaved);destroy(task);assert(vblankData.m14_numVsyncPerFrame==2);
 setupOk=true;cursorOk=false;source.standalone=false;
 task=static_cast<LagiMapParentTask*>(lagiAzelCreateMapParent(&parent,source));assert(task->getTask()->finished);destroy(task);
 std::puts("PASS: production Map parent opening/selection/field/world/closing, child status, native scroll/sounds/cadence and owned resource lifetime; live availability/dispatch/device not tested");
}
