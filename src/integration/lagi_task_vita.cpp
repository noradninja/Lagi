#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

s_task* taskListHead = nullptr;
int numActiveTask = 0;

// Normally owned by PDS.cpp. Keep the same type and default unpaused state
// until the Vita runtime brings the full engine-global layer online.
std::array<u8, 3> pauseEngine{};

void PrintDebugTaskHierarchy(s_task*) {}
void PrintDebugTasksHierarchy() {}
void PrintDebugTaskInfo(s_task*) {}
void PrintDebugTasksInfo() {}

void processTasks(s_task** ppTask)
{
    s_task* task = *ppTask;
    do {
        if (!task->isFinished()) {
            if (task->isPaused()) {
                task = *ppTask;
                if (!task) return;
                ppTask = &task->m0_pNextTask;
                task = *ppTask;
                continue;
            }
            if (!pauseEngine[0]) task->getWorkArea()->Update();
            task->getWorkArea()->Draw();
        }

        if (task->isFinished()) {
            task->m14_flags ^= TASK_FLAGS_DELETING;
            if (task->isDeleting()) {
                for (s_task* child = task->m4_pSubTask; child; child = child->m0_pNextTask)
                    child->markFinished();
            }
            if (task->m4_pSubTask) processTasks(&task->m4_pSubTask);

            if (task->isDeleting()) {
                task->getWorkArea()->Delete();
                --numActiveTask;
            } else {
                *ppTask = task->m0_pNextTask;
                delete task;
                task = *ppTask;
                if (!task) return;
                continue;
            }
        } else if (task->m4_pSubTask) {
            processTasks(&task->m4_pSubTask);
        }

        task = *ppTask;
        if (!task) return;
        ppTask = &task->m0_pNextTask;
        task = *ppTask;
    } while (task);
}

void runTasks() { if (taskListHead) processTasks(&taskListHead); }
void resetTasks() { taskListHead = nullptr; numActiveTask = 0; }

static p_workArea attachTask(p_workArea parent, p_workArea area, bool sibling)
{
    s_task* task = new s_task;
    assert(task);
    ++numActiveTask;
    s_task* anchor = parent->getTask();
    s_task** dst = sibling ? &anchor->m0_pNextTask : &anchor->m4_pSubTask;
    while (*dst) dst = &(*dst)->m0_pNextTask;
    *dst = task;
    task->m0_pNextTask = nullptr;
    task->m4_pSubTask = nullptr;
    task->m14_flags = 0;
    area->m_pTask = task;
    task->m_workArea = area;
    return task->getWorkArea();
}

p_workArea createSubTask(p_workArea parent, p_workArea area) { return attachTask(parent, area, false); }
p_workArea createSubTaskWithArg(p_workArea parent, p_workArea area) { return attachTask(parent, area, false); }
p_workArea createSiblingTaskWithArg(p_workArea area, p_workArea newArea) { return attachTask(area, newArea, true); }

s_workArea* createRootTask(p_workArea area)
{
    s_task* task = new s_task;
    assert(task);
    ++numActiveTask;
    taskListHead = task;
    task->m0_pNextTask = nullptr;
    task->m4_pSubTask = nullptr;
    task->m14_flags = 0;
    area->m_pTask = task;
    task->m_workArea = area;
    return area;
}

s_task* getTaskFromWorkArea(p_workArea area)
{
    return reinterpret_cast<s_task*>(reinterpret_cast<u8*>(area) - sizeof(s_task));
}

void dummyTaskInit(s_workArea* a) { std::printf("Unimplemented Init task for %s\n", a->getTask()->m_taskName); }
void dummyTaskInitWithArg(s_workArea* a, void*) { std::printf("Unimplemented Init task for %s\n", a->getTask()->m_taskName); }
void dummyTaskUpdate(s_workArea* a) { std::printf("Unimplemented Update task for %s\n", a->getTask()->m_taskName); }
void dummyTaskDraw(s_workArea* a) { std::printf("Unimplemented Draw task for %s\n", a->getTask()->m_taskName); }
void dummyTaskDelete(s_workArea* a) { std::printf("Unimplemented Delete task for %s\n", a->getTask()->m_taskName); }
