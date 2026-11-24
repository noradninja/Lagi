// Native 06055540..06055558 / shared 060A23C0. Named host state,
// including the separately authored Y override at native actor+1C8.
static void lagiUpdateGarilVariantPosition(sBTL_A5_VariantEnemy* actor) {
    const auto& center=actor->m158_formation->m2C_posBlock.m0_position;
    for(unsigned axis=0;axis<3;++axis) {
        lagi::NativeFormationSpringAxis motion={actor->m60_targetPosition[axis].asS32(),
            actor->m90_initialPosition[axis].asS32(),actor->m48[axis].asS32(),0,0};
        lagi::stepNativeFormationInterpolation(motion,center[axis].asS32(),actor->mA8_soundEffectId);
        actor->m48[axis]=motion.velocity;
        actor->m90_initialPosition[axis]=motion.current;
        actor->m30_position[axis]=motion.world;
    }
    actor->m30_position[1]=actor->m_lagiAltitude;
}

// Native 0605555A..0605557C / shared 060A2580: ordinary angular
// interpolation, without the signed-28-bit angular spring's wrapping.
static void lagiUpdateGarilVariantRotation(sBTL_A5_VariantEnemy* actor) {
    const auto quadrant=actor->m158_formation->mF4_entityPositions[actor->m1C_entityIndex];
    assert(quadrant<4);if(quadrant>=4)return;
    actor->m84_displayRotation[1]=readSaturnS32(g_BTL_A5->getSaturnPtr(0x060AD5A4+quadrant*4));
    sVec3_FP parentRotation;
    static_assert(sizeof(parentRotation)==12);
    // Same authored block data copy used by the existing Nanyd adapter.
    std::memcpy(&parentRotation,actor->m158_formation->m2C_posBlock.mC_pad,sizeof(parentRotation));
    for(unsigned axis=0;axis<3;++axis) {
        lagi::NativeFormationSpringAxis motion={actor->m84_displayRotation[axis].asS32(),
            actor->m9C_targetRotation[axis].asS32(),actor->m_lagiAngularVelocity[axis].asS32(),0,0};
        lagi::stepNativeFormationInterpolation(motion,parentRotation[axis].asS32(),actor->mA8_soundEffectId);
        actor->m_lagiAngularVelocity[axis]=motion.velocity;
        actor->m9C_targetRotation[axis]=motion.current;
        actor->m3C_rotation[axis]=motion.world;
    }
}
