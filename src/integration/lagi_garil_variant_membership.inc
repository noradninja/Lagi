// Native 0605557E..060555CE, before damage and actor state dispatch.
static void lagiUpdateGarilVariantMembership(sBTL_A5_VariantEnemy* actor) {
    auto* formation=actor->m158_formation;
    const unsigned index=actor->m1C_entityIndex;
    assert(index<3);
    if(index>=3)return;
    const auto quadrant=formation->mF4_entityPositions[index];
    const u8 mask=u8(1u<<index);
    auto& membership=formation->mF1_pad5[2]; // native +F3
    if(quadrant==gBattleManager->m10_battleOverlay->m4_battleEngine->m22C_dragonCurrentQuadrant)
        membership|=mask;
    else membership&=u8(~mask);
}
