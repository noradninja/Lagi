static void lagiUpdateNanydFormationTrail(sBTL_A5_FormationTask* formation,
                                          s_battleEngine* engine)
{
    formationUpdatePosition(&formation->m2C_posBlock,&engine->mC_battleCenter);
    std::array<sVec3_FP,18> trail;
    static_assert(sizeof(trail)==sizeof(formation->mF4_entityPositions));
    std::memcpy(trail.data(),formation->mF4_entityPositions,sizeof(trail));
    lagi::stepNativeFormationTrail(trail,formation->m2C_posBlock.m0_position);
    std::memcpy(formation->mF4_entityPositions,trail.data(),sizeof(trail));
}
