#include <cassert>
#include <cstdint>
#include "../src/integration/lagi_formation_task_signal.h"

struct Task {
    std::uint32_t m14_flags = 0;
    bool isFinished() const { return (m14_flags & 1) != 0; }
};
struct Entity {
    Task* task;
    Task* getTask() const { return task; }
};
struct Group {
    Entity** m0_entityArray;
    int mE_numEntities;
};
int main()
{
    Task live{}, finished{1}, noTaskTask{};
    Entity a{&live}, b{&finished}, c{nullptr}, d{&noTaskTask};
    Entity* members[] = {&a, nullptr, &b, &c, &d};
    Group group{members, 5};
    assert(lagiSignalFormationTasks(group, 8) == 2);
    assert(live.m14_flags == 8 && noTaskTask.m14_flags == 8);
    assert(finished.m14_flags == 1);
    assert(members[1] == nullptr && members[2] == nullptr && members[3] == nullptr);
    assert(lagiSignalFormationTasks(group, 8) == 2);
    assert(live.m14_flags == 8 && noTaskTask.m14_flags == 8);
    assert(lagiSignalFormationTasks(group, 0x10) == 2);
    assert(live.m14_flags == 0x18 && noTaskTask.m14_flags == 0x18);
}
