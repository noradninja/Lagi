// Native 0605F188..0605F1D8. The constructor seeds slots 0..16;
// slot 17 retains its zero-initialized work-area value until the first update.
static void lagiInitGarilDualSwarmLayout(sBTL_A5_DualFormation* formation) {
    formation->mE0_entityGroup.m8_dataTable=g_BTL_A5->getSaturnPtr(
        0x060B11C8+u8(formation->m27C_variant*0x78));
    formation->m1CC_dataTable2=g_BTL_A5->getSaturnPtr(0x060B13A8);
    static_assert(sizeof(sVec3_FP)==12);
    for(unsigned i=0;i<17;++i)
        std::memcpy(formation->mF4_entityPositions+i*sizeof(sVec3_FP),
            &formation->m2C_posBlock.m0_position,sizeof(sVec3_FP));
}
// Native 0605FBF4. Write the current head before the reverse history copy.
static void lagiUpdateGarilDualTrail(sBTL_A5_DualFormation* formation) {
    std::array<sVec3_FP,18> trail;
    static_assert(sizeof(trail)==sizeof(formation->mF4_entityPositions));
    std::memcpy(trail.data(),formation->mF4_entityPositions,sizeof(trail));
    lagi::stepNativeFormationTrail(trail,formation->m2C_posBlock.m0_position);
    std::memcpy(formation->mF4_entityPositions,trail.data(),sizeof(trail));
}
