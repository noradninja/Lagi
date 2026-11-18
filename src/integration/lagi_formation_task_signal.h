#pragma once

#include <cstdint>

// Original Saturn formation signaling targets the task header (+0x14), not
// an enemy work-area's own m14_flags.  The task is already owned by Azel;
// the adapter only supplies the typed access absent from the SH-2 layout.
// In particular, signal 0x8 is NOT the finished bit (0x1).
template <class EntityGroup>
inline int lagiSignalFormationTasks(EntityGroup& group, std::uint32_t signal)
{
    int notified = 0;
    for (int i = 0; i < group.mE_numEntities; ++i)
    {
        auto*& entity = group.m0_entityArray[i];
        if (!entity)
            continue;
        auto* task = entity->getTask();
        if (!task || task->isFinished())
        {
            entity = nullptr;
            continue;
        }
        task->m14_flags |= signal;
        ++notified;
    }
    return notified;
}
