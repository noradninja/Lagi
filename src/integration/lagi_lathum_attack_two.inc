// BTL_A5_3 mode2 060547B6..06054D0A. Reacting quadrant-facing parts
// emit rings; the controller owns the shared dragon displacement and damage.
static void lagiUpdateLathumAttackTwo(s_workAreaCopy* owner,LagiLathumRig& rig,
    s_battleEngine& engine,sVec3_FP& dragonPosition) {
    for(unsigned axis=0;axis<3;++axis) {
        rig.cameraEye[axis]=lagi::formationAdd(rig.cameraEye[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
        rig.cameraTarget[axis]=lagi::formationAdd(rig.cameraTarget[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
    }
    switch(rig.substate) {
    case 0:
        if(!engine.m188_flags.m2000)return;
        battleEngine_setCurrentCameraPositionPointer(&rig.cameraEye);
        battleEngine_setDesiredCameraPositionPointer(&rig.cameraTarget);
        battleEngine_enableAttackCamera();
        rig.cameraTarget=engine.mC_battleCenter;rig.cameraEye=engine.mC_battleCenter;
        rig.cameraEye[1]=dragonPosition[1];battleEngine_resetCameraInterpolation();rig.substate=1;return;
    case 1:
        playSystemSoundEffect(0x6D);rig.activeRingCount=0;
        for(unsigned group=0;group<rig.groups.size();++group)
            if(readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3EDD+group))==u8(engine.m22C_dragonCurrentQuadrant) && rig.groups[group].state) {
                rig.ring=lagiCreateLathumRing(owner,rig,u8(group),dragonPosition);
                ++rig.activeRingCount;
            }
        rig.controllerTimer=30;rig.substate=2;return;
    case 2: {
        if(rig.ring&&!rig.ring->getTask()->isFinished()) {
            rig.cameraTarget=static_cast<LagiLathumRing*>(rig.ring)->position;
            battleEngine_resetCameraInterpolation();
        } else rig.ring=nullptr;
        rig.controllerTimer=lagiLathumDamageWord(s32(rig.controllerTimer)-1);
        if(rig.controllerTimer>=0)return;
        rig.dragonDisplacement.zeroize();rig.dragonDisplacement[1]=s32(rig.activeRingCount)*0x3000;
        const auto speed=rig.dragonDisplacement[1].asS32();
        // Native06035686 is raw signed product/division, not the upstream
        // fixed-point MTH_Mul_5_6 translation. At most3 authored parts face a quadrant.
        const auto height=s32((int64_t(speed)*speed)/0x2C0)+0x5000;
        rig.cameraEye=dragonPosition;
        rig.cameraEye[1]=lagi::formationAdd(rig.cameraEye[1].asS32(),height);
        switch(u8(engine.m22C_dragonCurrentQuadrant)) {
        case 0:rig.cameraEye[2]=lagi::formationAdd(rig.cameraEye[2].asS32(),0x3000);break;
        case 1:rig.cameraEye[0]=lagi::formationAdd(rig.cameraEye[0].asS32(),0x3000);break;
        case 2:rig.cameraEye[2]=lagi::formationAdd(rig.cameraEye[2].asS32(),-0x3000);break;
        case 3:rig.cameraEye[0]=lagi::formationAdd(rig.cameraEye[0].asS32(),-0x3000);break;
        }
        battleEngine_resetCameraInterpolation();rig.substate=3;return;
    }
    case 3:
        for(unsigned axis=0;axis<3;++axis)
            engine.m164[axis]=lagi::formationAdd(engine.m164[axis].asS32(),rig.dragonDisplacement[axis].asS32());
        rig.cameraTarget=dragonPosition;battleEngine_resetCameraInterpolation();
        rig.dragonDisplacement[1]=lagi::formationAdd(rig.dragonDisplacement[1].asS32(),-0x160);
        if(engine.m164[1].asS32()>0)return;
        engine.m164[1]=0;
        applyDamageToDragon(gBattleManager->m10_battleOverlay->m18_dragon->m8C,
            s32(rig.activeRingCount)*92,dragonPosition,3,rig.dragonDisplacement,0);
        rig.substate=4;return;
    case 4:
        engine.m188_flags.m100_attackAnimationFinished=1;battleEngine_restoreCameraDefault();
        rig.mode=0;rig.substate=0;return;
    }
}
