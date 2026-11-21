// Original BTL_A5 0605BBd0..BDEA. Use the shared model loader and Valley
// target initializer; model/pose/animation indices are distinct bundle slots.
static bool lagiInitGarilSingleModels(sBTL_A5_SingleEnemy* actor,
                                     s_fileBundle* bundle, int variant)
{
    if (!lagiCreateBattleEnemyModels(actor,bundle,actor->m29_modelCount,
        actor->m2C_models,g_BTL_A5->getSaturnPtr(0x060B0028+variant*8),1)) {
        actor->mAC_lifeMeter->getTask()->markFinished();
        return false;
    }
    auto& model=actor->m2C_models[0];
    initAnimation(&model,bundle->getAnimation(0x70));
    const auto frames=model.m30_pCurrentAnimation ? model.m30_pCurrentAnimation->m4_numFrames : 0;
    setAnimationFrame(&model,performModulo2(frames-1u,randomNumber()));
    stepAnimation(&model);
    const auto bones=bundle->getModelHierarchy(0xC)->countNumberOfBones();
    if (!init3DModelRawData(actor,&actor->m15C_collisionModel,0,bundle,0xC,nullptr,
        bundle->getStaticPose(0x6C,bones),nullptr,nullptr)) {
        actor->getTask()->markFinished();
        actor->mAC_lifeMeter->getTask()->markFinished();
        return false;
    }
    stepAnimation(&actor->m15C_collisionModel);
    if (!lagiInitBattleEnemyModelTargets(actor,actor->m1C_entityIndex,actor->m2C_models,
        actor->m_lagiTargets,{0x060A3A4C,0x060A3A50,0x060A3A74,0x060A3A98})) {
        actor->mAC_lifeMeter->getTask()->markFinished();
        return false;
    }
    actor->mB6_numTargetables=model.m52_targetableCount;
    return true;
}

// Original 0605C2BC -> 060A23C0. m90 is the current interpolation state,
// group +8 is the table itself, and group +4 owns the parent position block.
static void lagiUpdateGarilSinglePosition(sBTL_A5_SingleEnemy* actor)
{
    const auto& group=actor->m158_formation->mE0_entityGroup;
    actor->m60_targetPosition=actor->m158_formation->m_lagiLayouts[actor->m158_formation->m_lagiLayout][actor->m1C_entityIndex];
    for (int axis=0;axis<3;++axis) {
        lagi::NativeFormationSpringAxis p{actor->m60_targetPosition[axis].asS32(),
            actor->m90_initialPosition[axis].asS32(),actor->m48_scaledAutoScroll[axis].asS32(),0,
            actor->m30_position[axis].asS32()};
        lagi::stepNativeFormationInterpolation(p,group.m4_positionBlock->m0_position[axis].asS32(),actor->mA8_soundEffectId);
        actor->m90_initialPosition[axis].m_value=p.current;
        actor->m48_scaledAutoScroll[axis].m_value=p.velocity;
        actor->m30_position[axis].m_value=p.world;
    }
}

void createExplosionAtPosition(sVec3_FP*,sVec3_FP*,int,u8);
static bool lagiUpdateGarilSingleDamage(sBTL_A5_SingleEnemy* actor)
{
    const LagiBattleModelDamageHooks hooks{nullptr,
        [](void*)->std::uint8_t {return gBattleManager->m10_battleOverlay->m4_battleEngine->m22C_dragonCurrentQuadrant;},
        [](void*) {return bool(gBattleManager->m10_battleOverlay->m4_battleEngine->m188_flags.m1000);},
        [](void*,std::uint32_t ea,s_workAreaCopy*,sBattleTargetable& target) {
            if (ea==0x060A3A4C) return;
            assert(ea==0x060A3A50 || ea==0x060A3A74 || ea==0x060A3A98);
            createExplosionAtPosition(target.mC,
                &gBattleManager->m10_battleOverlay->m4_battleEngine->m1A0_battleAutoScrollDelta,
                0x20000,ea==0x060A3A50 ? 1 : 0);
        },
        [](void*,s_workAreaCopy* owner,sBattleTargetable& t,int mode) {createHitSparkEffect(owner,t,mode);},
        [](void*,s_workAreaCopy* owner,std::int16_t n,sVec3_FP* p,int mode) {createDamageDisplayTask(owner,n,p,mode);}
    };
    return lagiUpdateBattleEnemyModelDamage({actor,actor->m14_flags,actor->mB2_health,
        actor->m_lagiTargets.healthFloor,actor->m_lagiPendingDamage,actor->m14C_lightColor,
        actor->mAC_lifeMeter->m31,actor->m30_position,actor->m2C_models,actor->m_lagiTargets},hooks);
}
