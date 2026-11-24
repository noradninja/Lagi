// BTL_A5_3 mode3 0605563C..06055A4A: orbit camera and repair parts in order.
static void lagiLathumRepairBurst(LagiLathumRig& rig,s_battleEngine& engine,
    const sVec3_FP& anchor) {
    auto* file=dramAllocatorEnd[readSaturnU8(rig.descriptor)].mC_fileBundle;
    const auto* quad=lagiNativeBattleParticleQuad(g_BTL_A5_3->getSaturnPtr(0x060A3B9C));
    auto acceleration=readSaturnVec3(g_BTL_A5_3->getSaturnPtr(0x060A3F70));
    for(unsigned particle=0;particle<16;++particle) {
        sVec3_FP velocity,position;
        velocity[0]=(randomNumber()&0xFFF)-0x7FF;
        velocity[2]=lagi::formationAdd((randomNumber()&0xFFF)-0x7FF,engine.m1A0_battleAutoScrollDelta[2].asS32());
        velocity[1]=randomNumber()&0xFFF;
        for(unsigned axis=0;axis<3;++axis)
            position[axis]=lagi::formationAdd(anchor[axis].asS32(),(randomNumber()&0xFFFF)-0x7FFF);
        const auto lifetime=(randomNumber()&15)+15;
        createParticleEffect(file,quad,&position,&velocity,&acceleration,0x30000,nullptr,lifetime);
    }
}
static void lagiUpdateLathumAttackThree(s_workAreaCopy* owner,LagiLathumRig& rig,
    s_battleEngine& engine,const sVec3_FP& dragonPosition) {
    rig.orbitAngle+=0x5B05B;
    rig.cameraEye=engine.mC_battleCenter;
    rig.cameraEye[0]=lagi::formationAdd(rig.cameraEye[0].asS32(),MTH_Mul(0xFA000,getSin(rig.orbitAngle>>16)).asS32());
    rig.cameraEye[2]=lagi::formationAdd(rig.cameraEye[2].asS32(),MTH_Mul(0xFA000,getCos(rig.orbitAngle>>16)).asS32());
    rig.cameraEye[1]=lagi::formationAdd(rig.cameraEye[1].asS32(),0x96000);
    for(unsigned axis=0;axis<3;++axis)
        rig.cameraTarget[axis]=lagi::formationAdd(rig.cameraTarget[axis].asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
    switch(rig.substate) {
    case 0:
        if(!engine.m188_flags.m2000)return;
        battleEngine_setCurrentCameraPositionPointer(&rig.cameraEye);
        battleEngine_setDesiredCameraPositionPointer(&rig.cameraTarget);battleEngine_enableAttackCamera();
        rig.orbitAngle=randomNumber();rig.cameraEye=engine.mC_battleCenter;
        rig.cameraEye[0]=lagi::formationAdd(rig.cameraEye[0].asS32(),MTH_Mul(0xFA000,getSin(rig.orbitAngle>>16)).asS32());
        rig.cameraEye[2]=lagi::formationAdd(rig.cameraEye[2].asS32(),MTH_Mul(0xFA000,getCos(rig.orbitAngle>>16)).asS32());
        rig.cameraEye[1]=lagi::formationAdd(dragonPosition[1].asS32(),0x32000);
        rig.cameraTarget=engine.mC_battleCenter;battleEngine_resetCameraInterpolation();rig.substate=1;return;
    case 1:
        for(unsigned group=0;group<rig.groups.size();++group)if(rig.groups[group].state) {
            rig.repairingGroup=u8(group);rig.controllerTimer=21;playSystemSoundEffect(0x6D);
            const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+group));
            sVec3_FP world;transformAndAddVec(rig.corePositions[anchor],world,cameraProperties2.m28[1]);
            rig.cameraTarget=world;lagiLathumRepairBurst(rig,engine,world);rig.substate=2;return;
        }
        rig.substate=10;return;
    case 2:
        rig.controllerTimer=lagiLathumDamageWord(s32(rig.controllerTimer)-1);
        if(rig.controllerTimer>=0)return;
        rig.groups[rig.repairingGroup].state=0;rig.substate=1;return;
    case 10:
        rig.selectedGroup=performModulo2(9,randomNumber());rig.groupCounter=-1;
        battleEngine_FlagQuadrantBitForSafety(0);engine.m188_flags.m100_attackAnimationFinished=1;
        battleEngine_restoreCameraDefault();rig.mode=0;rig.substate=0;return;
    }
}
