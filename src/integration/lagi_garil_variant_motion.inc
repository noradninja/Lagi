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
