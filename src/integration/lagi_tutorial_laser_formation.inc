// BTL_T0 0605863C/060586E0/06058740/060587CA. This authored formation's
// enemy-turn callback is the empty 060586DC; its reachable shared states are
// idle (0) and all-dead delay (7), not the attack states of other formations.
struct sLagiTutorialLaserFormation : s_workAreaTemplateWithCopy<sLagiTutorialLaserFormation> {
    std::array<s_workAreaCopy*,2> enemies{};
    npcFileDeleter* resources{};
    sVec3_FP position{},rotation{},velocity{},acceleration{},target{},current{};
    std::int8_t deadCount{};
    std::int16_t counter{};
    bool allDead{};
    static void Update(sLagiTutorialLaserFormation* s) {
        auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
        engine->m3B2_numBattleFormationRunning=lagiBattleDamageByte(
            std::uint8_t(engine->m3B2_numBattleFormationRunning)+1);
        if (s->deadCount>=2) {
            s->allDead=true;
            // Native resets these on every all-dead update, not only entry.
            engine->m3CC->m8=0;engine->m3CC->m0=0;
            engine->m3CC->m2=0;engine->m3CC->m4=0;
        }
        for (unsigned axis=0;axis<3;++axis) {
            lagi::NativeFormationSpringAxis state{s->target[axis].asS32(),
                s->current[axis].asS32(),s->velocity[axis].asS32(),
                s->acceleration[axis].asS32(),s->position[axis].asS32()};
            lagi::stepNativeFormationSpring(state,engine->mC_battleCenter[axis].asS32(),0xff,0xfff);
            s->current[axis].m_value=state.current;s->velocity[axis].m_value=state.velocity;
            s->acceleration[axis].m_value=state.acceleration;s->position[axis].m_value=state.world;
        }
        if (s->allDead) {
            const auto previous=s->counter;
            s->counter=lagiBattleDamageWord(std::uint32_t(previous)+1);
            if (previous>60) {s->counter=0;s->getTask()->markFinished();}
        } else {
            // Shared state 0 waits while attack flag 80 is active; otherwise
            // timer m8==1 invokes the authored empty callback. Do not consume
            // the timer, invent an attack or finish the formation here.
            if (engine->m188_flags.m80) return;
        }
    }
    static void Delete(sLagiTutorialLaserFormation* s) {
        if (s->resources) decreaseNPCRefCount(0x14);
    }
};
s_workAreaCopy* lagiCreateTutorialLaserFormation(const LagiTutorialLaserFormationSpawn& spawn)
{
    const auto& h=spawn.services;
    if (!spawn.parent || !h.damage.quadrant || !h.damage.displayWindow ||
        !h.damage.hitCallback || !h.damage.hitDisplay || !h.damage.damageDisplay ||
        !h.random || !h.sound || !h.recordDeath) return nullptr;
    static const sLagiTutorialLaserFormation::TypedTaskDefinition definition={
        nullptr,sLagiTutorialLaserFormation::Update,nullptr,sLagiTutorialLaserFormation::Delete};
    auto* s=createSubTaskWithCopy<sLagiTutorialLaserFormation>(spawn.parent,&definition);
    if (!s) return nullptr;
    s->resources=allocateNPC(s,0x14);
    if (!s->resources || !s->resources->m0_fileBundle) {
        s->getTask()->markFinished();return nullptr;
    }
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    engine->m3CC->m2=120;
    battleEngine_UpdateSub1Sub0(0);
    displayFormationName(6,0,9);
    for (int index=0;index<2;++index) {
        const auto offset=readSaturnVec3(spawn.positions+index*12);
        const LagiTutorialLaserEnemySpawn enemy{s,s->resources,s->position,s->rotation,
            s->deadCount,engine->m474_XPReceivedFromBattle,engine->m478_dyneReceivedFromBattle,
            offset,index,spawn.modelDescriptors,spawn.callbacks,spawn.services};
        s->enemies[index]=lagiCreateTutorialLaserEnemyTask(enemy);
        if (!s->enemies[index]) ++s->deadCount;
    }
    return s;
}
