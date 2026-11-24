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

// Native 06055030: consume the damage byte after damage processing.
static void lagiConsumeGarilVariantDamage(sBTL_A5_VariantEnemy* actor) {
    if(!actor->m1D5_wasDamaged)return;
    if(actor->m1D5_wasDamaged==1)actor->m14_flags&=~0x60000000u;
    actor->m1D5_wasDamaged=0;
}
// Native 06055146/06055380/0605540C: formation +F2 owns reaction bits.
static void lagiSetGarilVariantReaction(sBTL_A5_VariantEnemy* actor,bool active) {
    const unsigned index=actor->m1C_entityIndex;
    assert(index<3);if(index>=3)return;
    auto& reactions=actor->m158_formation->mF1_pad5[1];
    const u8 mask=u8(1u<<index);
    if(active)reactions|=mask;else reactions&=u8(~mask);
}
