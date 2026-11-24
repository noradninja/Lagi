u32& lagiA5ActorFlags(p_workArea actor);
void battleEngine_displayAttackName(int,int,int);
static bool lagiGarilDualTimerAfter(s16& timer,s16 threshold) {
    const s16 old=timer;const u16 bits=u16(u32(s32(old))+1);
    std::memcpy(&timer,&bits,2);return old>threshold;
}

void battleEngine_PlayAttackCamera(int);
void arachnoth_startDragonEvade(s16);
static void lagiGarilDualReleaseTurn(sBTL_A5_DualFormation* formation,u8 state) {
    auto* turn=gBattleManager->m10_battleOverlay->m4_battleEngine->m3CC;
    formation->m28_state=state;turn->m8=0;turn->m0=0;
}
static void lagiGarilDualMove(sBTL_A5_DualFormation* formation,int direction) {
    battleEngine_initiateEnemyMoveDragon(direction,60);lagiGarilDualReleaseTurn(formation,1);
}
static void lagiGarilDualRandomMove(sBTL_A5_DualFormation* formation) {
    lagiGarilDualMove(formation,(randomNumber()&1)?2:1);
}
// Native 0605FC2A: no replacement probability/selection policy.
static void lagiSelectGarilDualTurn(sBTL_A5_DualFormation* formation) {
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    const auto quadrant=engine->m22C_dragonCurrentQuadrant;
    if(quadrant==0) {
        if(gBattleManager->m10_battleOverlay->m18_dragon->m1C0_statusModifiers&1) {
            lagiGarilDualRandomMove(formation);return;
        }
        if(formation->m288_allSingleDead){lagiGarilDualRandomMove(formation);return;}
        auto& group=formation->m1D0_entityGroup2;
        bool eligible=false;
        for(int i=0;i<group.mE_numEntities;++i) {
            s32 dead;std::memcpy(&dead,formation->m1E4_pad6+i*sizeof(dead),sizeof(dead));
            if(!dead && lagiVisitFormationActor(group,i,[](p_workArea){}) &&
                !(formation->m1E3_flag2&(1u<<i)))eligible=true;
        }
        if(!eligible){lagiGarilDualReleaseTurn(formation,0);return;}
        for(int i=0;i<group.mE_numEntities;++i) {
            s32 dead;std::memcpy(&dead,formation->m1E4_pad6+i*sizeof(dead),sizeof(dead));
            if(!dead)lagiVisitFormationActor(group,i,[](p_workArea actor){lagiGarilSingleActorFlags(actor)|=0x0C000000u;});
        }
        battleEngine_SetBattleMode(eBattleModes::m9);
        battleEngine_FlagQuadrantBitForAttack(1);formation->m28_state=6;return;
    }
    if(quadrant==1 || quadrant==3) {
        const auto bits=formation->m_lagiMoveCounter++;
        const int old=bits<128?int(bits):int(bits)-256;
        if(old>=2){formation->m_lagiMoveCounter=0;lagiGarilDualMove(formation,quadrant==1?2:1);return;}
        if(formation->m284_allSwarmDead){lagiGarilDualRandomMove(formation);return;}
        playSystemSoundEffect(0x68);
        auto& group=formation->mE0_entityGroup;
        lagiReadyFormationActors(group,3,0x08000000u,0x04000000u,lagiA5ActorFlags);
        if(!lagiAnyFormationActorFlags(group,0x04000000u,lagiA5ActorFlags)) {
            lagiGarilDualRandomMove(formation);return;
        }
        battleEngine_SetBattleMode(eBattleModes::m7);
        battleEngine_FlagQuadrantBitForAttack(quadrant==3?8:2);formation->m28_state=2;return;
    }
    if(quadrant==2) {
        if(formation->m284_allSwarmDead){lagiGarilDualRandomMove(formation);return;}
        playSystemSoundEffect(0x68);
        auto& group=formation->mE0_entityGroup;
        if(!lagiReadyFormationActors(group,group.mE_numEntities,0,0x0C000000u,lagiA5ActorFlags)) {
            lagiGarilDualRandomMove(formation);return;
        }
        group.m8_dataTable=g_BTL_A5->getSaturnPtr(0x060B12B8+u8(formation->m27C_variant*0x78));
        arachnoth_startDragonEvade(60);battleEngine_FlagQuadrantBitForAttack(4);
        formation->m28_state=4;
    }
}
