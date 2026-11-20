// Host execution of the production A5 formation-state adapter. Compile
// with MSVC or GCC; no Vita runtime or Azel source replacement required.
#include <cassert>
#include <cstdint>
using u32 = std::uint32_t;

struct Task {
    bool finished = false;
    bool isFinished() const { return finished; }
    void markFinished() { finished = true; }
};
struct Actor {
    Task task;
    std::uint32_t flags = 0;
    Task* getTask() { return &task; }
};
using p_workArea = Actor*;
struct Group {
    Actor** m0_entityArray;
    int mE_numEntities;
    int mF_deadCount = 0;
};
struct Controls { int m8 = 1, m0 = 1, m2 = 1, m4 = 1; };
struct EngineFlags {
    bool m20_battleIntroRunning = false;
    bool m2000 = false;
    bool m10000 = false;
    bool m100_attackAnimationFinished = false;
};
struct s_battleEngine {
    EngineFlags m188_flags;
    Controls* m3CC;
};
struct sBTL_A5_FormationTask {
    Group mE0_entityGroup{};
    int m28_state = 3;
    int m1D4_flag = 0;
    int mF0_flag = 0;
    int m26_counter2 = 8;
};
enum class eBattleModes { m7 = 7 };
static int modeCalls = 0;
static int displayCalls = 0;
void battleEngine_displayAttackName(int textIndex, int duration, int option)
{
    assert(textIndex == 3 && duration == 40 && option == 0);
    ++displayCalls;
}
void battleEngine_SetBattleMode(eBattleModes mode)
{
    assert(mode == eBattleModes::m7);
    ++modeCalls;
}
void lagiA5SignalActorFlags(p_workArea actor, u32 flags)
{
    actor->flags |= flags;
}

#include "../src/integration/lagi_a5_scatter_formation.inc"

int main()
{
    Controls controls;
    s_battleEngine engine{{}, &controls};
    Actor lead, survivor, second;
    Actor* members[] = {&lead, &survivor, &second};
    sBTL_A5_FormationTask formation;
    formation.mE0_entityGroup = {members, 3, 0};

    // No lead kill, or battle intro running: no special transition.
    lagiA5BeginQueenScatter(&formation, &engine);
    assert(formation.m28_state == 3 && modeCalls == 0);
    formation.mF0_flag = 1;
    engine.m188_flags.m20_battleIntroRunning = true;
    lagiA5BeginQueenScatter(&formation, &engine);
    assert(formation.m28_state == 3 && modeCalls == 0);

    // Lead dies, but the two surviving actors must receive flag 8.
    engine.m188_flags.m20_battleIntroRunning = false;
    lead.task.finished = true;
    formation.mE0_entityGroup.mF_deadCount = 1;
    lagiA5BeginQueenScatter(&formation, &engine);
    assert(formation.m28_state == 9 && formation.m1D4_flag == 1);
    assert(modeCalls == 1 && controls.m8 == 0 && controls.m0 == 0);
    lagiA5UpdateQueenScatter(&formation, &engine);
    assert(formation.m28_state == 9 && survivor.flags == 0);
    engine.m188_flags.m2000 = true;
    lagiA5UpdateQueenScatter(&formation, &engine);
    assert(formation.m28_state == 11 && formation.m26_counter2 == 0);
    assert(members[0] == nullptr && lead.flags == 0);
    assert(survivor.flags == 8 && second.flags == 8);
    assert(displayCalls == 1);
    assert(engine.m188_flags.m10000);
    assert(engine.m188_flags.m100_attackAnimationFinished);

    // Queen-loss path must be one-shot, without a second mode transition.
    lagiA5BeginQueenScatter(&formation, &engine);
    assert(modeCalls == 1 && displayCalls == 1);

    // The ten-leader variant disables queen-loss Scatter.
    Actor* lone[] = {&survivor};
    sBTL_A5_FormationTask ten;
    ten.mE0_entityGroup = {lone, 1, 0};
    ten.mF0_flag = 1;
    ten.m1D4_flag = 1;
    lagiA5BeginQueenScatter(&ten, &engine);
    assert(ten.m28_state == 3 && modeCalls == 1);

    // Ordinary all-enemies-dead cleanup is not a Scatter transition.
    sBTL_A5_FormationTask allDead;
    allDead.mE0_entityGroup = {lone, 1, 1};
    allDead.mF0_flag = 1;
    lagiA5BeginQueenScatter(&allDead, &engine);
    assert(allDead.m28_state == 3 && modeCalls == 1);
}
