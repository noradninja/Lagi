#pragma once
#include <cstdint>

// Native group traversal with a typed actor-field accessor. A task's scheduler
// flags are not its combat flags; neither depends on Saturn work-area offsets.
template<class Group, class ActorFlags>
void lagiSignalBattleFormation(Group& group, unsigned bit, ActorFlags flags)
{
    const std::uint32_t mask=std::uint32_t(1) << (bit & 31u);
    for (int i=0;i<group.mE_numEntities;++i) {
        auto*& actor=group.m0_entityArray[i];
        if (!actor || actor->getTask()->isFinished()) actor=nullptr;
        else flags(actor)|=mask;
    }
}
