// BTL_A3_2 06057E94..06058076. The reconstructed phase only evaluated
// one discarded scalar and returned, preventing every later attack phase.
static void lagiArachnothDevourApproach(sArachnothFormation* pThis)
{
    const auto& dragon = gBattleManager->m10_battleOverlay->m18_dragon->m8_position;
    pThis->m2F0_eatDragonVelocity += MTH_Mul(dragon - pThis->m308_eatDragonPosition, 0x28F);
    pThis->m2FC_eatDragonAcceleration += pThis->m2F0_eatDragonVelocity;
    pThis->m2FC_eatDragonAcceleration -= MTH_Mul(pThis->m2FC_eatDragonAcceleration, 0x1999);
    pThis->m308_eatDragonPosition += pThis->m2FC_eatDragonAcceleration;
    pThis->m2F0_eatDragonVelocity.zeroize();
    if (pThis->m2C4_currentAttackDelay == 0x19) {
        pThis->m230 = dragon;
        ++pThis->m2C0_currentAttackState;
    }
}

// 06058268..060582FC: frame 79 installs the close-up, then continues the
// common frame increment (it must not return forever at frame 79).
static void lagiArachnothDevourCloseup(sArachnothFormation* pThis)
{
    pThis->m2CC = gBattleManager->m10_battleOverlay->m18_dragon->m8_position;
    pThis->m2CC[1] += 0xA000;
    const int angle = (pThis->m278_targetedQuadrant << 10) & 0xFFF;
    pThis->m2CC[0] += MTH_Mul(0x4000, getSin(angle));
    pThis->m2CC[2] += MTH_Mul(0x4000, getCos(angle));
    battleEngine_setCurrentCameraPositionPointer(&pThis->m2CC);
    battleEngine_setDesiredCameraPositionPointer(&pThis->m224_translation);
    battleEngine_resetCameraInterpolation();
}
