#pragma once

#include <cstdint>

// A native formation group owns work-area pointers; do not assume the
// work-area has the Saturn struct layout. Filter using the ordinary Azel
// task API, then delegate the area actor's work-area flag write to its
// typed receiver. Native A5: 0605D0C8..D0E0.
// Shared indexed access for native selection and attack transitions. Task
// validity belongs to the task header; flags belong to the typed actor.
template <class EntityGroup, class VisitActor>
inline bool lagiVisitFormationActor(EntityGroup& group,int index,VisitActor&& visit)
{
    if(index<0 || index>=group.mE_numEntities)return false;
    auto*& actor=group.m0_entityArray[index];
    if(!actor)return false;
    auto* task=actor->getTask();
    if(!task || task->isFinished()){
        actor=nullptr;
        return false;
    }
    visit(actor);
    return true;
}

template <class EntityGroup, class SignalActor>
inline int lagiSignalFormationActors(EntityGroup& group,
                                     std::uint32_t bits,
                                     SignalActor&& signalActor)
{
    int signaled=0;
    for(int i=0;i<group.mE_numEntities;++i)
        if(lagiVisitFormationActor(group,i,[&](auto* actor){signalActor(actor,bits);}))
            ++signaled;
    return signaled;
}

// Native A5 060A2AB0: transition only actors that were readied. The supplied
// flag accessor returns a typed work-area flag reference, never task flags.
template <class EntityGroup, class ActorFlags>
inline int lagiTransitionFormationActors(EntityGroup& group,
    std::uint32_t required,std::uint32_t clear,std::uint32_t set,ActorFlags&& actorFlags)
{
    int transitioned=0;
    for(int i=0;i<group.mE_numEntities;++i)
        lagiVisitFormationActor(group,i,[&](auto* actor){
            auto& flags=actorFlags(actor);
            if(flags&required){flags=(flags&~clear)|set;++transitioned;}
        });
    return transitioned;
}
