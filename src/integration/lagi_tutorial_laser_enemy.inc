// BTL_T0 06058996..06058C2E. This is the actual authored update body,
// with named actor references and typed native service boundaries.
void lagiUpdateTutorialLaserEnemy(const LagiTutorialLaserEnemyView& actor,
    const LagiTutorialLaserEnemyHooks& hooks)
{
    auto& damage=actor.damage;
    void* context=hooks.damage.context;
    lagiUpdateTutorialModelMotion(actor.motion);
    stepAnimation(&damage.models[0]);
    if (lagiUpdateBattleEnemyModelDamage(damage,hooks.damage)) {
        if (damage.health<=0) {
            lagiRetireBattleEnemyModelTargets(damage.owner,damage.flags,damage.models,nullptr);
            hooks.sound(context,0x65);
            hooks.recordDeath(context); // Native 0605D6E4 -> statistic selector 8.
            actor.experience=lagi::formationAdd(actor.experience,16);
            actor.dyne=lagi::formationAdd(actor.dyne,10);
        } else {
            // Native 0609F568 with callback zero: latch once, then sound/state
            // are still performed on every live hit, not only the first one.
            damage.flags|=0x40000000u;
            hooks.sound(context,0x66);
            actor.state=1;
        }
    }
    switch (actor.state) {
    case 0:
        for (unsigned axis=0;axis<3;++axis) {
            const auto jitter=std::int32_t(hooks.random(context)&0xffffu)-0x8000;
            actor.motion.targetPosition[axis].m_value=lagi::formationAdd(
                actor.authoredOffset[axis].asS32(),jitter);
        }
        break;
    case 1:
        damage.flags&=~0x60000000u;
        for (unsigned axis=0;axis<3;++axis) {
            const auto impulse=std::int32_t(hooks.random(context)&0x3fffffu)-0x200000;
            actor.motion.angularAcceleration[axis].m_value=lagi::formationAdd(
                actor.motion.angularAcceleration[axis].asS32(),impulse);
        }
        actor.state=0;
        break;
    case 2:
        createBoneDebris(damage.owner,actor.resources,&damage.models[0],
            &actor.motion.worldPosition,&actor.motion.worldRotation,nullptr,nullptr,
            -44,0x1fffff,0,0,0,0x10000,0);
        hooks.hideDrawing(context);
        actor.state=3;
        break;
    case 3: {
        const auto previous=actor.deathTimer;
        actor.deathTimer=lagiBattleDamageWord(std::uint32_t(previous)+1u);
        if (previous>30) {
            actor.deathTimer=0;
            const auto count=std::uint8_t(actor.formationDeadCount)+1u;
            actor.formationDeadCount=static_cast<std::int8_t>((count&0xffu)<128
                ? int(count&0xffu) : int(count&0xffu)-256);
            damage.owner->getTask()->markFinished();
        }
        break;
    }
    default:
        break;
    }
    // The original optional 1A debug print does not drive state or timing.
}
