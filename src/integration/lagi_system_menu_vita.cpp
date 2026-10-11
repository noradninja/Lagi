#include "lagi/lagi_azel_upstream_prelude.h"

namespace {
// No temporary UI. The native settings entry is disabled until its real
// controls/sound task is ported. Direct callers get a finished child rather
// than upstream's assertion or a null child that can strand its parent.
struct LagiUnavailableSystemTask : s_workAreaTemplate<LagiUnavailableSystemTask>
{
    static void Init(LagiUnavailableSystemTask* self) { self->getTask()->markFinished(); }
    static TypedTaskDefinition* getTypedTaskDefinition()
    {
        static TypedTaskDefinition definition = {&Init, nullptr, nullptr, nullptr};
        return &definition;
    }
};
}

p_workArea createSystemMenuTask(p_workArea parent)
{
    return createSubTask<LagiUnavailableSystemTask>(parent);
}
