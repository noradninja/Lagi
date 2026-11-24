// Native 0605F10C group identity/parent blocks; second block at +1FC.
static void lagiInitGarilDualGroups(sBTL_A5_DualFormation* formation) {
    formation->mE0_entityGroup.mC_formationNameIndex=15;
    formation->m1D0_entityGroup2.mC_formationNameIndex=17;
    formation->mE0_entityGroup.m4_positionBlock=&formation->m2C_posBlock;
    formation->m1D0_entityGroup2.m4_positionBlock=&formation->m_lagiSecondaryBlock;
    formation->m_lagiSecondaryBlock.m0_position[1]=-0x13000;
    formation->m_lagiSecondaryBlock.m30_target[1]=-0x13000;
    formation->m_lagiSecondaryBlock.m60_currentInterp[1]=-0x13000;
}
static p_workArea lagiCreateGarilDualSingle(sBTL_A5_DualFormation* formation,int index) {
    const LagiGarilSingleGroupBinding binding{&formation->m1D0_entityGroup2,
        &formation->m1E0_pad5[0],&formation->m1E0_pad5[2],formation->m1E4_pad6,formation,
        [](void* context,int entity)->sVec3_FP {
            auto* dual=static_cast<sBTL_A5_DualFormation*>(context);
            return dual->m_lagiSinglePositions[entity];
        }};
    return lagiCreateGarilSingleActor(formation,binding,index,1);
}
// Native factory 0605F338..0605F35A overrides constructor block Y values.
static void lagiFinishGarilDualMotion(sBTL_A5_DualFormation* formation) {
    formation->m2C_posBlock.m78_interpRate=0xA3D;
    formation->m_lagiSecondaryBlock.m78_interpRate=0xA3D;
    formation->m2C_posBlock.m0_position[1]=0x13000;
    formation->m2C_posBlock.m30_target[1]=0x13000;
    formation->m2C_posBlock.m60_currentInterp[1]=0x13000;
    formation->m_lagiSecondaryBlock.m0_position[1]=0;
    formation->m_lagiSecondaryBlock.m30_target[1]=0;
    formation->m_lagiSecondaryBlock.m60_currentInterp[1]=0;
}

static void lagiInitGarilDualSingleLayout(sBTL_A5_DualFormation* formation) {
    for(unsigned i=0;i<3;++i)
        formation->m_lagiSinglePositions[i]=readSaturnVec3(formation->m1D0_entityGroup2.m8_dataTable+i*12);
}
