// Original BTL_A5_3 mode4, 06054D34..060554CC.
static sVec3_FP lagiLathumCameraOffset(const sVec3_FP& center,s32 x,s32 y,s32 z) {
    auto result=center;const s32 offset[3]={x,y,z};
    for(unsigned axis=0;axis<3;++axis) result[axis]=lagi::formationAdd(center[axis].asS32(),offset[axis]);
    return result;
}
static p_workArea lagiLathumDialogue(s_workAreaCopy* owner,u32 address) {
    // The existing shared producer's factory only casts its formation parent
    // to workArea. Use its typed task directly, with the same zero init flag.
    return createSubTaskWithArg<sFormationIntroEffectTask>(owner,g_BTL_A5_3->getSaturnPtr(address));
}
static void lagiLathumEmergenceBurst(LagiLathumRig& rig,s_battleEngine& engine) {
    const auto* quad=lagiNativeBattleParticleQuad(g_BTL_A5_3->getSaturnPtr(0x060A3C60));
    auto* file=dramAllocatorEnd[readSaturnU8(rig.descriptor)].mC_fileBundle;
    for(unsigned i=0;i<100;++i) {
        sVec3_FP velocity,position;
        velocity[0]=(randomNumber()&0xFFF)-0x7FF;
        velocity[2]=lagi::formationAdd(lagi::formationAdd(randomNumber()&0xFFF,
            engine.m1A0_battleAutoScrollDelta[2].asS32()),-0x7FF);
        velocity[1]=randomNumber()&0xFFF;
        position=engine.mC_battleCenter;
        position[0]=lagi::formationAdd(position[0].asS32(),(randomNumber()&0xFFFFF)-0x7FFFF);
        position[2]=lagi::formationAdd(position[2].asS32(),(randomNumber()&0xFFFFF)-0x7FFFF);
        position[1]=lagi::formationAdd(position[1].asS32(),randomNumber()&0x3FFFF);
        const auto lifetime=performModulo2(60,randomNumber());
        createParticleEffect(file,quad,&position,&velocity,nullptr,0x50000,nullptr,lifetime);
    }
}
static void lagiUpdateLathumEmergence(s_workAreaCopy* owner,LagiLathumRig& rig,
    s_battleEngine& engine,s_battleGrid& grid,const sVec3_FP& dragonPosition) {
    for(unsigned axis=0;axis<3;++axis) {
        rig.cameraEye[axis]=lagi::formationAdd(rig.cameraEye[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
        rig.cameraTarget[axis]=lagi::formationAdd(rig.cameraTarget[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
    }
    if(readKeyboardToggle(0x81)){rig.modelPhase=1;rig.substate=6;}
    if(rig.cameraTransition&&rig.cameraTransition->getTask()->isFinished())rig.cameraTransition=nullptr;
    if(rig.dialogue&&rig.dialogue->getTask()->isFinished())rig.dialogue=nullptr;
    auto resetReferences=[&] {
        grid.mE4_currentCameraReferenceCenter=grid.m134_desiredCameraPosition;
        grid.mF0_currentCameraReferenceForward=grid.m140_desiredCameraTarget;
        grid.m108_deltaCameraPosition.zeroize();grid.m114_deltaCameraTarget.zeroize();
    };
    auto decrement=[&] {rig.controllerTimer=lagiLathumDamageWord(s32(rig.controllerTimer)-1);};
    switch(rig.substate) {
    case 0:
        if(!engine.m3D0)return;
        engine.m3D0->clearDeleteMethod();engine.m3D0->getTask()->markFinished();
        battleEngine_restoreCameraDefault();battleEngine_resetBattleCameraPreset();resetReferences();
        engine.m3D0=nullptr;battleEngine_SetBattleMode(m7);
        battleEngine_enableAttackCamera();resetReferences();
        battleEngine_setCurrentCameraPositionPointer(&rig.cameraEye);
        battleEngine_setDesiredCameraPositionPointer(&rig.cameraTarget);
        rig.cameraEye=lagiLathumCameraOffset(engine.mC_battleCenter,0x1E000,0x11000,s32(0xFFF55ED2));
        rig.cameraTarget=lagiLathumCameraOffset(engine.mC_battleCenter,s32(0xFFF94000),-0x52000,0x7C000);
        battleEngine_resetCameraInterpolation();rig.controllerTimer=90;rig.substate=1;return;
    case 1:
        decrement();if(rig.controllerTimer>0)return;
        rig.cameraTransition=lagiCreateNativeVectorTransition(owner,&rig.cameraTarget,
            lagiLathumCameraOffset(engine.mC_battleCenter,0x4000,0xD0000,0x74000),180);
        rig.substate=2;return;
    case 2:
        if(rig.cameraTransition)return;
        rig.cameraEye=lagiLathumCameraOffset(engine.mC_battleCenter,0,0x117000,0xD5ED2);
        battleEngine_resetCameraInterpolation();
        rig.cameraTransition=lagiCreateNativeVectorTransition(owner,&rig.cameraEye,
            lagiLathumCameraOffset(engine.mC_battleCenter,0x22000,0x198000,0x119E00),270);
        playSystemSoundEffect(0x6D);lagiLathumEmergenceBurst(rig,engine);
        rig.modelPhase=1;rig.controllerTimer=30;rig.substate=3;return;
    case 3:
        decrement();if(rig.controllerTimer>=0)return;
        rig.dialogue=lagiLathumDialogue(owner,0x060A3F04);rig.substate=4;return;
    case 4:
        if(rig.dialogue||rig.cameraTransition)return;
        rig.cameraTransition=lagiCreateNativeVectorTransition(owner,&rig.cameraEye,
            lagiLathumCameraOffset(engine.mC_battleCenter,0xE0000,0x58000,s32(0xFFF9BED3)),270);
        lagiCreateNativeVectorTransition(owner,&rig.cameraTarget,
            lagiLathumCameraOffset(engine.mC_battleCenter,0,0,-0x52000),270);
        rig.dialogue=lagiLathumDialogue(owner,0x060A3F28);rig.substate=5;return;
    case 5:
        if(rig.cameraTransition)return;
        rig.cameraEye=dragonPosition;rig.cameraTarget=engine.mC_battleCenter;rig.substate=6;return;
    case 6:
        if(rig.dialogue)return;
        engine.m188_flags.m100_attackAnimationFinished=1;battleEngine_restoreCameraDefault();
        rig.mode=0;rig.substate=0;return;
    }
}
