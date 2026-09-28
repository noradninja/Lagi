#include <cstdio>
#include "lagi/azel_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

namespace lagi::azel {

struct SmokeTask final : s_workAreaTemplate<SmokeTask>
{
    int updates = 0;
    int draws = 0;

    static void Init(SmokeTask* self) { std::printf("[Azel] root task init\n"); self->updates = self->draws = 0; }
    static void UpdateTask(SmokeTask* self) { ++self->updates; }
    static void DrawTask(SmokeTask* self) { ++self->draws; }
    static void DeleteTask(SmokeTask*) { std::printf("[Azel] root task delete\n"); }

    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition def = { Init, UpdateTask, DrawTask, DeleteTask };
        return &def;
    }
};

static SmokeTask* gSmokeTask = nullptr;

bool runtime_smoke_init()
{
    initHeap();
    resetTasks();
    gSmokeTask = createRootTask<SmokeTask>();
    if (!gSmokeTask) return false;
    std::printf("[Azel] native task runtime linked; root=%p\n", static_cast<void*>(gSmokeTask));
    return true;
}

void runtime_smoke_frame()
{
    runTasks();
    if (gSmokeTask && gSmokeTask->updates == 1)
        std::printf("[Azel] first task frame: update=%d draw=%d\n", gSmokeTask->updates, gSmokeTask->draws);
}

} // namespace lagi::azel
