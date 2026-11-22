// BTL_A5_3 modeB 06055A4C..06055C3A. Native reward ownership stays with
// the existing life-meter service; no direct results/progression shortcuts.
static void lagiUpdateLathumDeath(LagiLathumRig& rig,s_battleEngine& engine,
    const sVec3_FP& dragonPosition) {
    rig.localPosition[2]=lagi::formationAdd(rig.localPosition[2].asS32(),0x2000);
    rig.cameraEye=dragonPosition;rig.cameraTarget=rig.position;
    rig.cameraTarget[2]=lagi::formationAdd(rig.cameraTarget[2].asS32(),0x64000);
    auto& model=rig.models[0];
    if(model.m16_previousAnimationFrame!=33)stepAnimation(&model);
    switch(rig.substate) {
    case 0:
        rig.modelPhase=2;setAnimationFrame(&model,0);rig.lifeMeter->m31|=1;rig.lifeMeter->m31|=8;
        playSystemSoundEffect(0x6E);
        for(auto& group:rig.groups)for(auto& target:group.targets)deleteTargetable(&target);
        rig.substate=1;return;
    case 1:
        if(!battleEngine_isBattleIntroFinished())return;
        battleEngine_SetBattleMode(m2);rig.substate=2;return;
    case 2:
        if(!engine.m188_flags.m2000)return;
        battleEngine_setCurrentCameraPositionPointer(&rig.cameraEye);
        battleEngine_setDesiredCameraPositionPointer(&rig.cameraTarget);battleEngine_enableAttackCamera();
        rig.controllerTimer=240;rig.substate=3;return;
    case 3:
        rig.controllerTimer=lagiLathumDamageWord(s32(rig.controllerTimer)-1);
        if(rig.controllerTimer>=0)return;
        rig.substate=10;return;
    case 10:
        battleEngine_restoreCameraDefault();engine.m188_flags.m100_attackAnimationFinished=1;
        rig.controllerTimer=15;rig.substate=11;return;
    case 11:
        rig.controllerTimer=lagiLathumDamageWord(s32(rig.controllerTimer)-1);
        if(rig.controllerTimer<=0)rig.hideBody=1;
        return;
    }
}
