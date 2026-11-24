// Native 0605F48E..0605F590. Phase zero follows the live head; any
// nonzero attack phase scrolls both snapshots. No camera helper is recreated.
static void lagiUpdateGarilDualCamera(sBTL_A5_DualFormation* formation,s_battleEngine* engine) {
    for(unsigned axis=0;axis<3;++axis)
        formation->mA8_position[axis]=lagi::formationAdd(formation->mA8_position[axis].asS32(),
            engine->m1A0_battleAutoScrollDelta[axis].asS32());
    if(formation->m280_phase==0) {
        formation->mC0_position2=formation->m2C_posBlock.m0_position;
        formation->mC0_position2[2]=lagi::formationSubtract(formation->mC0_position2[2].asS32(),0x12000);
    } else {
        for(unsigned axis=0;axis<3;++axis)
            formation->mC0_position2[axis]=lagi::formationAdd(formation->mC0_position2[axis].asS32(),
                engine->m1A0_battleAutoScrollDelta[axis].asS32());
    }
}
