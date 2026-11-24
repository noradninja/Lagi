// BTL_A5_3 06055EF8..060561FE. Preserve phase -> mode -> damage order.
static void lagiLathumAmbientParticle(LagiLathumRig& rig,bool dying) {
    if(randomNumber()&31)return;
    const auto group=performModulo2(9,randomNumber());
    const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+group));
    sVec3_FP world;transformAndAddVec(rig.corePositions[anchor],world,cameraProperties2.m28[0]);
    auto velocity=readSaturnVec3(g_BTL_A5_3->getSaturnPtr(dying?0x060A3FBC:0x060A3FB0));
    createParticleEffect(dramAllocatorEnd[readSaturnU8(rig.descriptor)].mC_fileBundle,
        lagiNativeBattleParticleQuad(g_BTL_A5_3->getSaturnPtr(0x060A3B9C)),&world,&velocity,
        nullptr,dying?0x80000:0x30000,nullptr,105);
}
static void lagiUpdateLathumRig(s_workAreaCopy* owner,LagiLathumRig& rig,
    s_battleEngine& engine,s_battleGrid& grid,s_battleDragon& dragon) {
    for(unsigned axis=0;axis<3;++axis)
        rig.position[axis]=lagi::formationAdd(rig.localPosition[axis].asS32(),engine.mC_battleCenter[axis].asS32());
    auto activeFormation=[&] {
        const auto count=(unsigned(u8(engine.m3B2_numBattleFormationRunning))+1)&255u;
        engine.m3B2_numBattleFormationRunning=s8(count<128?int(count):int(count)-256);
    };
    switch(rig.modelPhase) {
    case 0:activeFormation();stepAnimation(&rig.emergenceModel);break;
    case 1:activeFormation();stepAnimation(&rig.models[0]);lagiLathumAmbientParticle(rig,false);break;
    case 2:lagiLathumAmbientParticle(rig,true);break;
    }
    switch(rig.mode) {
    case 0:lagiSelectLathumTurn(rig,engine);break;
    case 1:lagiUpdateLathumAttackOne(owner,rig,engine,dragon.m8_position);break;
    case 2:lagiUpdateLathumAttackTwo(owner,rig,engine,dragon.m8_position);break;
    case 3:lagiUpdateLathumAttackThree(owner,rig,engine,dragon.m8_position);break;
    case 4:lagiUpdateLathumEmergence(owner,rig,engine,grid,dragon.m8_position);break;
    case 5:lagiUpdateLathumReaction(owner,rig,engine,dragon.m8_position);break;
    case 11:lagiUpdateLathumDeath(rig,engine,dragon.m8_position);break;
    }
    if(rig.modelPhase==1)lagiUpdateLathumDamage(owner,rig,engine);
}
