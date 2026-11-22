// Host task cleanup: targets retire before task heaps and C++ model storage.
// A completed native death keeps its existing meter reward request intact.
static void lagiDeleteGarilVariantActor(sBTL_A5_VariantEnemy* actor) {
    const bool alreadyRetired=(actor->m14_flags&0x10000000u)!=0;
    lagiRetireBattleEnemyModelTargets(actor,actor->m14_flags,actor->m2C_models,nullptr);
    lagiSetGarilVariantReaction(actor,false);
    if(!alreadyRetired)lagiRetireBattleEnemyMeter(actor->mAC_lifeMeter);
}
