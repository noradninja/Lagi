#include <cassert>
#include <cstdint>
#include "../src/integration/lagi_formation_actor_signal.h"

struct Task { std::uint32_t flags = 0; bool isFinished() const { return flags & 1; } };
struct Actor {
    Task* task;
    std::uint32_t workAreaFlags = 0;
    Task* getTask() const { return task; }
};
struct Group { Actor** m0_entityArray; int mE_numEntities; };
int main()
{
    Task live{}, finished{1}, another{};
    Actor a{&live}, b{&finished}, c{nullptr}, d{&another};
    Actor* actors[] = {&a, nullptr, &b, &c, &d};
    Group group{actors, 5};
    auto signal = [](Actor* actor, std::uint32_t bits) { actor->workAreaFlags |= bits; };
    assert(lagiSignalFormationActors(group, 8, signal) == 2);
    assert(a.workAreaFlags == 8 && d.workAreaFlags == 8);
    assert(live.flags == 0 && another.flags == 0 && finished.flags == 1);
    assert(b.workAreaFlags == 0 && actors[2] == nullptr && actors[3] == nullptr);
    assert(lagiSignalFormationActors(group, 8, signal) == 2);
    assert(lagiSignalFormationActors(group, 0x20, signal) == 2);
    assert(a.workAreaFlags == 0x28 && d.workAreaFlags == 0x28);
    a.workAreaFlags=0x0C000000u;d.workAreaFlags=0x10;
    live.flags=0x04000000u;another.flags=0x04000000u;
    auto flags=[](Actor* actor)->std::uint32_t& {return actor->workAreaFlags;};
    assert(lagiTransitionFormationActors(group,0x04000000u,0x04000000u,0x02000000u,flags)==1);
    assert(a.workAreaFlags==0x0A000000u&&d.workAreaFlags==0x10);
    assert(live.flags==0x04000000u&&another.flags==0x04000000u);
    assert(lagiTransitionFormationActors(group,0x04000000u,0x04000000u,0x02000000u,flags)==0);
    assert(!lagiVisitFormationActor(group,-1,[](Actor*){}));
    assert(!lagiVisitFormationActor(group,5,[](Actor*){}));

}
