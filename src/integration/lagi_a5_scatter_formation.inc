#include "lagi_formation_actor_signal.h"

// Native A5 BTL_A5.PRG 0605CDF4..CE4E / 0605D0A0..D124.
// Task-header finished checks and work-area flag 8 are separate operations.
void lagiA5SignalActorFlags(p_workArea actor, u32 flags);
void battleEngine_displayAttackName(int textIndex, int duration, int param3);

static void lagiA5BeginQueenScatter(sBTL_A5_FormationTask* formation,
                                     s_battleEngine* engine)
{
    auto& group = formation->mE0_entityGroup;
    // The ten-leader formation sets m1D4 initially, disabling this path.
    // A kill of the lead enemy sets mF0. Do not trigger a separate Scatter
    // when the ordinary all-dead path is already processing cleanup.
    if (formation->m28_state == 0xB || formation->m28_state == 9 ||
        formation->m1D4_flag || !formation->mF0_flag ||
        engine->m188_flags.m20_battleIntroRunning ||
        group.mF_deadCount >= group.mE_numEntities)
        return;

    formation->m1D4_flag = 1;
    battleEngine_SetBattleMode(eBattleModes::m7);
    engine->m3CC->m8 = 0;
    engine->m3CC->m0 = 0;
    engine->m3CC->m2 = 0;
    engine->m3CC->m4 = 0;
    formation->m28_state = 9;
}

static void lagiA5UpdateQueenScatter(sBTL_A5_FormationTask* formation,
                                      s_battleEngine* engine)
{
    if (!engine->m188_flags.m2000)
        return;

    lagiSignalFormationActors(formation->mE0_entityGroup, 8u,
        [](p_workArea actor, std::uint32_t flags) {
            lagiA5SignalActorFlags(actor, static_cast<u32>(flags));
        });

    // Original A5 0605D0F6 calls 06070A7E(3, 40, 0).
    // The existing Azel attack-name service owns the original text table,
    // bottom-of-screen display task, and presentation lifetime.
    battleEngine_displayAttackName(3, 40, 0);
    engine->m188_flags.m10000 = 1;
    engine->m188_flags.m100_attackAnimationFinished = 1;
    formation->m28_state = 0xB;
    formation->m26_counter2 = 0;
}
