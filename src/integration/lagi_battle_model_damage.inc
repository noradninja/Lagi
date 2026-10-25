// BTL_T0 0609F0CA..0609F43E. Named actor view keeps Saturn offsets out of
// host work areas. The same shared pipeline can service other model enemies.
static std::int16_t lagiBattleDamageWord(std::uint32_t bits)
{
    const auto low=bits&0xffffu;
    return static_cast<std::int16_t>(low<0x8000u ? std::int32_t(low)
                                              : std::int32_t(low)-0x10000);
}
static std::int8_t lagiBattleDamageByte(std::int16_t word)
{
    const auto low=std::uint16_t(word)&0xffu;
    return static_cast<std::int8_t>(low<0x80u ? int(low) : int(low)-0x100);
}
bool lagiUpdateBattleEnemyModelDamage(LagiBattleModelDamageView actor,
    const LagiBattleModelDamageHooks& hooks)
{
    if (actor.flags&0x10000000u) return false;
    bool damaged=false;
    for (unsigned modelIndex=0;modelIndex<actor.models.size();++modelIndex) {
        auto& model=actor.models[modelIndex];
        if (!model.m52_targetableCount) continue;
        // 0609F25C: flatten the already transformed hotpoint positions into
        // the stable task-owned array referenced by native targetables.
        unsigned targetIndex=0;
        for (unsigned bone=0;bone<model.m50_targetBoneCount;++bone) {
            const auto count=(*model.m40)[bone].m4_count;
            for (unsigned point=0;point<count;++point)
                model.m58_targetablePositions[targetIndex++]=model.m44_hotpointData[bone][point];
        }
        // 0609F2E8: native short table entries are stored as bytes in m60.
        for (unsigned target=0;target<model.m52_targetableCount;++target)
            model.m54_targetables[target].m60=lagiBattleDamageByte(
                actor.targets.hitboxValues[hooks.quadrant(hooks.context)]);
        for (unsigned row=0;row<actor.targets.auxiliaryCount;++row) {
            const auto& override=actor.targets.auxiliaryData[row];
            if (int(override.modelIndex)!=int(modelIndex)) continue;
            // Authored row is a signed model/target pair plus four words.
            if (override.targetIndex<0 || unsigned(override.targetIndex)>=model.m52_targetableCount) {
                actor.owner->getTask()->markFinished();return damaged;
            }
            model.m54_targetables[override.targetIndex].m60=lagiBattleDamageByte(
                override.hitboxValues[hooks.quadrant(hooks.context)]);
        }
        // Dead actors still refresh positions/table values but don't consume
        // hit flags. Native tests this once per model, not once per target.
        if (actor.flags&0x80000000u) continue;
        for (unsigned targetIndex=0;targetIndex<model.m52_targetableCount;++targetIndex) {
            auto& target=model.m54_targetables[targetIndex];
            if (!(target.m50_flags&0x80000u)) continue;
            target.m50_flags&=~0xa0000u;
            damaged=true;
            // 0609F348: callbacks happen before health subtraction.
            actor.flags|=0x800000u;
            actor.lightColor=std::uint8_t(target.m60);
            const int kind=target.m5E_impactForce;
            if (kind>=0 && kind<4) {
                const auto ea=actor.targets.damageCallbackEA[kind];
                if (ea) hooks.hitCallback(hooks.context,ea,actor.owner,target);
            }
            actor.health=lagiBattleDamageWord(std::uint32_t(actor.health)-std::uint32_t(target.m58));
            if (actor.health>=0) hooks.hitDisplay(hooks.context,actor.owner,target,0);
            if (actor.healthFloor!=0 && actor.health<=actor.healthFloor)
                actor.health=actor.healthFloor;
            actor.lifeMeterFlags|=2;
            actor.pendingDamage=lagiBattleDamageWord(std::uint32_t(actor.pendingDamage)+std::uint32_t(target.m58));
        }
    }
    // 0609F3C2: death flushes even zero/negative accumulation. While alive,
    // engine flag 1000 and a positive accumulator are both required.
    if (actor.health<=0 || (hooks.displayWindow(hooks.context) && actor.pendingDamage>0)) {
        hooks.damageDisplay(hooks.context,actor.owner,actor.pendingDamage,&actor.worldPosition,1);
        actor.pendingDamage=0;
    }
    return damaged;
}
