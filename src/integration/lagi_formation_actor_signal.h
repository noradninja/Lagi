#pragma once

#include <cstdint>

// A native formation group owns work-area pointers; do not assume the
// work-area has the Saturn struct layout. Filter using the ordinary Azel
// task API, then delegate the area actor's work-area flag write to its
// typed receiver. Native A5: 0605D0C8..D0E0.
template <class EntityGroup, class SignalActor>
inline int lagiSignalFormationActors(EntityGroup& group,
                                     std::uint32_t bits,
                                     SignalActor&& signalActor)
{
    int signaled = 0;
    for (int i = 0; i < group.mE_numEntities; ++i)
    {
        auto*& actor = group.m0_entityArray[i];
        if (!actor)
            continue;
        auto* task = actor->getTask();
        if (!task || task->isFinished())
        {
            actor = nullptr;
            continue;
        }
        signalActor(actor, bits);
        ++signaled;
    }
    return signaled;
}
