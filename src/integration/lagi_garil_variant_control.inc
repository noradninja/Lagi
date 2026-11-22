void battleEngine_displayAttackName(int,int,int);
static u32& lagiGarilVariantActorFlags(p_workArea actor) {
    return static_cast<sBTL_A5_VariantEnemy*>(actor)->m14_flags;
}
// Native 06054C2E: membership mask selects attack; otherwise alternate moves.
static void lagiSelectGarilVariantTurn(sBTL_A5_FormationTask* formation) {
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    auto& group=formation->mE0_entityGroup;
    bool selected=false;
    for(int i=0;i<group.mE_numEntities;++i)
        lagiVisitFormationActor(group,i,[&](p_workArea actor){
            if(formation->mF1_pad5[2]&(1u<<i)) {
                lagiGarilVariantActorFlags(actor)|=0x04000000u;selected=true;
            }
        });
    if(selected) {
        if(lagiAnyFormationActorFlags(group,0x04000000u,lagiGarilVariantActorFlags)) {
            battleEngine_SetBattleMode(eBattleModes::m7);
            battleEngine_FlagQuadrantBitForAttack(1u<<engine->m22C_dragonCurrentQuadrant);
            formation->m28_state=6;
        } else {
            formation->m28_state=3;engine->m3CC->m8=0;engine->m3CC->m0=0;
        }
        return;
    }
    auto& moveCounter=formation->mF4_entityPositions[4]; // native +F8
    const auto old=moveCounter++;
    const auto flag=old==0 ? 1u : 2u;
    lagiSignalFormationActors(group,flag,[](p_workArea actor,u32 bits){lagiGarilVariantActorFlags(actor)|=bits;});
    if(old!=0)moveCounter=0;
    battleEngine_SetBattleMode(eBattleModes::m7);
    formation->m28_state=4;
}
static bool lagiGarilVariantCounterAfter(s16& counter,s16 threshold) {
    const s16 old=counter;
    const u16 bits=u16(u32(s32(old))+1);std::memcpy(&counter,&bits,2);
    return old>threshold;
}
