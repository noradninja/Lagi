#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include <unordered_map>
#include "lagi/native_task_disposal.h"
using u8=std::uint8_t;using u32=std::uint32_t;
static int liveArrays=0;
void* operator new[](std::size_t n){auto* p=std::malloc(n);if(!p)throw std::bad_alloc();++liveArrays;return p;}
void operator delete[](void* p) noexcept{if(p){--liveArrays;std::free(p);}}
struct s_heapNode{u32 m_magic;s_heapNode* m_nextNode;u32 m_size;void* getUserData(){return this+1;}};
struct s_task;
struct s_workArea{
    s_task* task=nullptr;int id=0,updates=0,draws=0,deletes=0;
    void* allocation=nullptr;void* explicitAllocation=nullptr;
    s_workArea* child=nullptr;
    s_task* getTask(){return task;}
    void Update(){++updates;}void Draw(){++draws;}void Delete();
};
constexpr u32 TASK_FLAGS_DELETING=4;
struct s_task{
    s_task* m0_pNextTask=nullptr; s_task* m4_pSubTask=nullptr;
    u32 m14_flags=0;s_workArea* area=nullptr;s_heapNode* m_heapNode=nullptr;
    s_workArea* getWorkArea(){return area;}
    bool isFinished(){return m14_flags&1;}bool isPaused(){return m14_flags&2;}
    bool isDeleting(){return m14_flags&4;}void markFinished(){m14_flags|=1;}
};
std::array<u8,3> pauseEngine{};
#include "heap.inc"
#include "scheduler.inc"
static std::vector<int> deletionOrder;
void s_workArea::Delete(){
    ++deletes;deletionOrder.push_back(id);
    assert(task->m_heapNode && static_cast<u8*>(allocation)[0]==id);
    if(child){assert(child->deletes==1);assert(static_cast<u8*>(child->allocation)[0]==child->id);}
    // Delete callbacks may explicitly free a middle allocation and allocate
    // again. The final owner-list drain must release the new allocation too.
    freeHeapForTask(this,explicitAllocation);explicitAllocation=nullptr;
    assert(allocateHeapForTask(this,19));
}
static s_task* attach(s_workArea& area,int id){
    auto* task=new s_task;task->area=&area;area.task=task;area.id=id;
    area.allocation=allocateHeapForTask(&area,32);
    static_cast<u8*>(area.allocation)[0]=static_cast<u8>(id);
    area.explicitAllocation=allocateHeapForTask(&area,48);
    allocateHeapForTask(&area,64);++numActiveTask;return task;
}
int main(){
    for(int encounter=0;encounter<256;++encounter){
        s_workArea parent,child,survivor;
        taskListHead=attach(parent,1);auto* root=taskListHead;
        root->m4_pSubTask=attach(child,2);parent.child=&child;
        root->m0_pNextTask=attach(survivor,3);survivor.task->m14_flags=2;
        root->markFinished();deletionOrder.clear();
        runTasks(); // child then parent callbacks; allocations stay live
        assert((deletionOrder==std::vector<int>{2,1}));
        assert(numActiveTask==1 && liveArrays==9);
        assert(survivor.updates==0 && survivor.draws==0);
        runTasks(); // disposal pass, child first; survivor untouched
        assert(taskListHead==survivor.task && liveArrays==3);
        assert(survivor.task->m_heapNode && static_cast<u8*>(survivor.allocation)[0]==3);
        survivor.task->markFinished();runTasks();runTasks();
        assert(!taskListHead && numActiveTask==0 && liveArrays==0);
        assert(parent.deletes==1 && child.deletes==1 && survivor.deletes==1);
    }
    // WORLD-style opt-in: concrete destruction must happen after BOTH native
    // callback passes and the final heap drain, exactly once, child first.
    struct OwnedArea : s_workArea {
        int* destroyed;
        std::vector<int> storage=std::vector<int>(1024,7);
        explicit OwnedArea(int& count):destroyed(&count){}
        ~OwnedArea(){assert(!task->m_heapNode&&deletes==1);++*destroyed;deletionOrder.push_back(id+10);}
    };
    for(int visit=0;visit<256;++visit){
        int destroyed=0;
        auto* parent=new OwnedArea(destroyed);auto* child=new OwnedArea(destroyed);
        taskListHead=attach(*parent,1);parent->task->m4_pSubTask=attach(*child,2);parent->child=child;
        auto dispose=[](void* p){delete static_cast<OwnedArea*>(p);};
        lagi::tasks::register_work_area_disposer(parent,dispose);
        lagi::tasks::register_work_area_disposer(child,dispose);
        taskListHead->markFinished();deletionOrder.clear();
        runTasks();assert(destroyed==0&&numActiveTask==0&&liveArrays==6);
        assert((deletionOrder==std::vector<int>{2,1}));
        runTasks();assert(destroyed==2&&!taskListHead&&!liveArrays);
        assert((deletionOrder==std::vector<int>{2,1,12,11}));
        assert(lagi::tasks::workAreaDisposers.empty());
    }
    puts("PASS 256 registered concrete-owner lifecycles: vectors destroyed after callbacks/heaps, child first, no retained disposal entries; legacy owners unchanged");
    puts("PASS: production scheduler/heap bodies reclaim owned allocations over 256 nested encounter lifecycles; child-first callbacks, delayed disposal, explicit frees/reallocations and paused survivor ownership retained. Mocked task records, not Vita combat.");
}
