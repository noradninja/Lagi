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
