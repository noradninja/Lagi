// Native 0605BF68/0605C01C: targetable +8 is its live WORLD position,
// not the model's view-space hotpoint array. Lead record offset C8 = index 2.
static const sVec3_FP* lagiGarilSingleAttackPosition(sBTL_A5_SingleEnemy* actor)
{
    const auto& model=actor->m2C_models[0];
    const unsigned index=actor->m1BC_isLead2==1 ? 2u : 0u;
    assert(model.m54_targetables && index<model.m52_targetableCount);
    if(!model.m54_targetables || index>=model.m52_targetableCount)return nullptr;
    return model.m54_targetables[index].m8;
}

static void lagiGarilSingleBeginCharge(sBTL_A5_SingleEnemy* actor)
{
    const auto* source=lagiGarilSingleAttackPosition(actor);
    if(!source)return;
    const bool lead=actor->m1BC_isLead2==1;
    lagiCreateNativeBattleChargeQuad(actor,source,&actor->m28_state,1,
        g_BTL_A5->getSaturnPtr(lead ? 0x060AFE44 : 0x060AFAFC),6);
}

static void lagiGarilSingleFire(sBTL_A5_SingleEnemy* actor)
{
    const auto* source=lagiGarilSingleAttackPosition(actor);
    if(!source)return;
    auto velocity=gBattleManager->m10_battleOverlay->m4_battleEngine->m1A0_battleAutoScrollDelta;
    velocity[2]=0; // Native copies scroll Z then subtracts that same scroll Z.
    const bool lead=actor->m1BC_isLead2==1;
    lagiCreateNativeHomingBattleProjectile(actor,*source,velocity,
        g_BTL_A5->getSaturnPtr(lead ? 0x060AFE44 : 0x060AFAFC),6,
        lead ? 0x21 : 0x18,lead ? 8 : 0);
}
