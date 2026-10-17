#include <cassert>
#include <cstdio>
#include <vector>
using s32=int;
static std::vector<int> calls;
struct Task{bool finished=false;void markFinished(){finished=true;calls.push_back(6);}};
struct s_workArea{Task task;Task* getTask(){return &task;}};
using p_workArea=s_workArea*;
template<class T>struct s_workAreaTemplate:s_workArea {
 struct TypedTaskDefinition {void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};
};
static bool failAllocation=false,soundReady=false;
template<class T>T* createSubTask(p_workArea parent,const typename T::TypedTaskDefinition* definition){
 assert(parent && !definition->init && !definition->update && definition->draw && !definition->remove);
 return failAllocation?nullptr:new T{}; // Native base new zeroes its work area.
}
static int cameraProperties2;
static void reset3dEngine(){calls.push_back(3);}
static void resetCameraProperties2(int* camera){assert(camera==&cameraProperties2);calls.push_back(4);}
static void initTownGrid(){calls.push_back(5);}
static bool isSoundLoadingFinished(){calls.push_back(2);return soundReady;}
#include "native.inc"
static p_workArea expectedParent;
static void first(p_workArea p){assert(p==expectedParent);calls.push_back(1);}
static void second(p_workArea p){assert(p==expectedParent);calls.push_back(7);}
int main(){
 s_workArea parent;expectedParent=&parent;
 auto* task=static_cast<sBattleMainTask*>(createBattleMainTask(&parent,first,second));
 assert(task && task->m0_status==0 && calls.empty());
 battleMainTask_Draw(task);assert((calls==std::vector<int>{1}) && task->m0_status==1);
 for(unsigned i=0;i<5;++i)battleMainTask_Draw(task);
 assert(task->m0_status==1 && !task->getTask()->finished && calls.size()==6);
 soundReady=true;battleMainTask_Draw(task);assert(task->m0_status==2 && !task->getTask()->finished);
 calls.clear();battleMainTask_Draw(task);
 assert((calls==std::vector<int>{3,4,5,7,6}) && task->getTask()->finished);
 delete task;
 calls.clear();failAllocation=true;
 assert(!createBattleMainTask(&parent,first,second) && calls.empty());
 std::puts("PASS: actual native battle main factory/Draw preserves parent, zero-state entry, one music callback, sound wait, 3D/camera/grid/engine ordering and completion; mocked services, not battle execution");
}
