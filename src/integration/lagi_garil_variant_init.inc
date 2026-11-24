// Native 06054DF8 constructor: shared model descriptor AD5C0,
// animation 48, collision hierarchy 8/static pose 44, shared target binding.
static bool lagiInitGarilVariantModels(sBTL_A5_VariantEnemy* actor,s_fileBundle* bundle) {
    if(!lagiCreateBattleEnemyModels(actor,bundle,actor->m29_modelCount,
        actor->m2C_models,g_BTL_A5->getSaturnPtr(0x060AD5C0),1)) {
        actor->mAC_lifeMeter->getTask()->markFinished();return false;
    }
    auto& model=actor->m2C_models[0];
    initAnimation(&model,bundle->getAnimation(0x48));
    const auto frames=model.m30_pCurrentAnimation?model.m30_pCurrentAnimation->m4_numFrames:0;
    setAnimationFrame(&model,performModulo2(frames-1u,randomNumber()));
    stepAnimation(&model);
    const auto bones=bundle->getModelHierarchy(8)->countNumberOfBones();
    if(!init3DModelRawData(actor,&actor->m160_collisionModel,0,bundle,8,nullptr,
        bundle->getStaticPose(0x44,bones),nullptr,nullptr)) {
        actor->getTask()->markFinished();actor->mAC_lifeMeter->getTask()->markFinished();return false;
    }
    stepAnimation(&actor->m160_collisionModel);
    if(!lagiInitBattleEnemyModelTargets(actor,actor->m1C_entityIndex,actor->m2C_models,
        actor->m_lagiTargets,{0x060A3A4C,0x060A3A50,0x060A3A74,0x060A3A98})) {
        actor->mAC_lifeMeter->getTask()->markFinished();return false;
    }
    actor->mB6_numTargetables=model.m52_targetableCount;
    actor->m_lagiTargets.hitboxValues={45,5,5,5};
    return true;
}
