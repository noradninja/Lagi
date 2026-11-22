// Original BTL_A5_3 mode5, 060554CE..06055604.
static void lagiUpdateLathumReaction(s_workAreaCopy* owner,LagiLathumRig& rig,
    s_battleEngine& engine,sVec3_FP& dragonPosition) {
    for(unsigned axis=0;axis<3;++axis)
        rig.cameraTarget[axis]=lagi::formationAdd(rig.cameraTarget[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
    switch(rig.substate) {
    case 0: {
        if(!battleEngine_isBattleIntroFinished())return;
        battleEngine_SetBattleMode(m2);
        battleEngine_setCurrentCameraPositionPointer(&dragonPosition);
        battleEngine_setDesiredCameraPositionPointer(&rig.cameraTarget);
        battleEngine_enableAttackCamera();
        const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+rig.selectedGroup));
        transformAndAddVec(rig.corePositions[anchor],rig.cameraTarget,cameraProperties2.m28[1]);
        rig.substate=1;return;
    }
    case 1:
        if(!engine.m188_flags.m2000)return;
        rig.dialogue=lagiCreateNativeBattleSpeech(owner,10,0,g_BTL_A5_3->getSaturnPtr(0x060A3F64));
        rig.substate=2;return;
    case 2:
        if(rig.dialogue&&!rig.dialogue->getTask()->isFinished())return;
        rig.substate=3;return;
    case 3:
        engine.m188_flags.m100_attackAnimationFinished=1;rig.mode=0;rig.substate=0;return;
    }
}
