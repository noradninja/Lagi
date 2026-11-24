// BTL_A5_3 mode1 06054504..060547B4. Fire from intact groups facing
// the dragon, then retain the authored signed90-to-negative completion wait.
static void lagiUpdateLathumAttackOne(s_workAreaCopy* owner,LagiLathumRig& rig,
    s_battleEngine& engine,sVec3_FP& dragonPosition) {
    for(unsigned axis=0;axis<3;++axis) {
        rig.cameraEye[axis]=lagi::formationAdd(rig.cameraEye[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
        rig.cameraTarget[axis]=lagi::formationAdd(rig.cameraTarget[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
    }
    switch(rig.substate) {
    case 0:
        if(!engine.m188_flags.m2000)return;
        battleEngine_setCurrentCameraPositionPointer(&dragonPosition);
        battleEngine_setDesiredCameraPositionPointer(&rig.cameraTarget);
        battleEngine_enableAttackCamera();rig.cameraTarget=engine.mC_battleCenter;
        rig.substate=1;return;
    case 1:
        playSystemSoundEffect(0x6D);
        for(unsigned group=0;group<rig.groups.size();++group)
            if(readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3EDD+group))==u8(engine.m22C_dragonCurrentQuadrant) && !rig.groups[group].state)
                lagiCreateLathumProjectile(owner,rig,u8(group),engine,dragonPosition);
        rig.controllerTimer=90;rig.substate=2;return;
    case 2:
        rig.controllerTimer=lagiLathumDamageWord(s32(rig.controllerTimer)-1);
        if(rig.controllerTimer>=0)return;
        engine.m188_flags.m100_attackAnimationFinished=1;rig.mode=0;rig.substate=0;return;
    }
}
