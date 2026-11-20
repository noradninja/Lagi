// BTL_T0 060587F0..06058994, after native task allocation. Caller supplies
// the authored group position and the file-slot-14 bundle, not a proxy model.
bool lagiInitTutorialLaserEnemy(const LagiTutorialLaserEnemyInitView& actor,
    std::int32_t entityIndex, const sVec3_FP& authoredOffset,
    sSaturnPtr modelDescriptors, const std::array<std::uint32_t,4>& callbacks,
    std::uint32_t (*random)(void*), void* randomContext)
{
    if (!actor.bundle) {actor.owner->getTask()->markFinished();return false;}
    actor.owner->m0_fileBundle=actor.bundle;
    actor.entityIndex=entityIndex;
    actor.health=300;
    actor.lifeMeter=createEnemyLifeMeterTask(&actor.motion.worldPosition,300,&actor.health,-1);
    if (!actor.lifeMeter) {actor.owner->getTask()->markFinished();return false;}
    if (!lagiCreateBattleEnemyModels(actor.owner,actor.bundle,actor.modelCount,
        actor.models,modelDescriptors,1)) {
        // Life meters are dragon children, not necessarily actor children.
        // Do not leave one referring to the health of a failed owner task.
        actor.lifeMeter->getTask()->markFinished();return false;
    }
    auto& model=actor.models[0];
    initAnimation(&model,actor.bundle->getAnimation(0x9c));
    const std::uint32_t frames=model.m30_pCurrentAnimation
        ? model.m30_pCurrentAnimation->m4_numFrames : 0;
    // Original MAIN 06035550 is DIV0U remainder, not signed %, a quotient,
    // or rng % frames. Keep the native modulo helper's zero-divisor behavior.
    const auto frame=performModulo2(frames-1u,random(randomContext));
    setAnimationFrame(&model,lagi::formationSignedBits(frame));
    stepAnimation(&model);
    if (!lagiInitBattleEnemyModelTargets(actor.owner,std::uint32_t(entityIndex),
        actor.models,actor.targets,callbacks)) {
        actor.lifeMeter->getTask()->markFinished();return false;
    }
    actor.targets.hitboxValues={30,1,10,1};
    actor.motion.targetPosition=authoredOffset;
    actor.motion.currentPosition=authoredOffset;
    actor.interpolationRate=0xf5c;
    actor.motion.worldRotation[0].m_value=0;
    actor.motion.worldRotation[1].m_value=0x08000000;
    actor.motion.worldRotation[2].m_value=0;
    actor.motion.currentRotation=actor.motion.worldRotation;
    actor.motion.targetRotation=actor.motion.worldRotation;
    return true;
}
