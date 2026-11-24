// Native BTL_A5 0605F3C8..0605F460: cleanup and defeat are independent.
static void lagiUpdateGarilDualCompletion(sBTL_A5_DualFormation* formation)
{
    formationCleanupEntityArray(&formation->mE0_entityGroup);
    formationCleanupEntityArray(&formation->m1D0_entityGroup2);
    if (formationAreAllEntitiesDead(&formation->mE0_entityGroup)) {
        formation->m284_allSwarmDead=1;
        battleEngine_FlagQuadrantBitForSafety(0);
        battleEngine_FlagQuadrantBitForSafety(1);
        battleEngine_FlagQuadrantBitForDanger(0xE);
    }
    if (formationAreAllEntitiesDead(&formation->m1D0_entityGroup2)) {
        formation->m288_allSingleDead=1;
        battleEngine_FlagQuadrantBitForSafety(0);
        battleEngine_FlagQuadrantBitForSafety(4);
        battleEngine_FlagQuadrantBitForDanger(1);
    }
    if (formation->m284_allSwarmDead==1 && formation->m288_allSingleDead==1) {
        formation->m28_state=10;
        auto* turn=gBattleManager->m10_battleOverlay->m4_battleEngine->m3CC;
        turn->m8=0;turn->m0=0;turn->m2=0;turn->m4=0;
    }
}
