#pragma once
#include <cstdint>
struct sEntityGroup;
struct sVec3_FP;
// Native actor factories receive parent task and group separately. These
// pointers refer to named host fields, never Saturn offsets in work areas.
struct LagiGarilSingleGroupBinding {
    sEntityGroup* group=nullptr;
    std::uint8_t* activeMask=nullptr;
    std::uint8_t* reactionMask=nullptr;
    std::uint8_t* deadWords=nullptr;
    void* positionContext=nullptr;
    sVec3_FP (*targetPosition)(void*,int)=nullptr;
};

struct s_workArea;
struct s_workAreaCopy;
// Shared native 0605BBD0 factory: parent owns the task; binding owns its
// formation identity and live motion/lifecycle state.
s_workArea* lagiCreateGarilSingleActor(s_workAreaCopy* parent,
    const LagiGarilSingleGroupBinding& binding,int entityIndex,int isLead);
// Typed actor work-area flags, distinct from task scheduler flags.
std::uint32_t& lagiGarilSingleActorFlags(s_workArea* actor);
