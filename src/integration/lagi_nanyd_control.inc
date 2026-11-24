u32& lagiA5ActorFlags(p_workArea actor);
void arachnoth_startDragonEvade(s16 duration); // Existing shared mode-A entry.

// Native 0605D1F4. The host's spare named byte carries native move-counter
// state (+1D0); no native work-area offsets are used on the C++ formation.
static void lagiSelectNanydAttack(sBTL_A5_FormationTask* formation,s_battleEngine* engine)
{
    const int quadrant=engine->m22C_dragonCurrentQuadrant;
    auto& group=formation->mE0_entityGroup;
    switch(quadrant){
    case 0:
        battleEngine_initiateEnemyMoveDragon((randomNumber()&1) ? 2 : 1,0x3C);
        formation->m28_state=4;
        break;
    case 1:
    case 3:
        if(s8(formation->mF1_pad5[0]++)>=2){
            formation->mF1_pad5[0]=0;
            battleEngine_initiateEnemyMoveDragon(quadrant==1 ? 2 : 1,0x3C);
            formation->m28_state=4;
        }else{
            playSystemSoundEffect(0x68);
            lagiReadyFormationActors(group,3,0x08000000u,0x04000000u,lagiA5ActorFlags);
            if(lagiAnyFormationActorFlags(group,0x04000000u,lagiA5ActorFlags)){
                battleEngine_SetBattleMode(eBattleModes::m7);
                battleEngine_FlagQuadrantBitForAttack(quadrant==1 ? 2 : 8);
                formation->m28_state=5;
            }else formation->m28_state=3;
        }
        break;
    case 2:
        playSystemSoundEffect(0x68);
        lagiReadyFormationActors(group,group.mE_numEntities,0,0x0C000000u,lagiA5ActorFlags);
        group.m8_dataTable=g_BTL_A5->getSaturnPtr(formation->m1D4_flag ? 0x060B0F90 : 0x060B0E14);
        // Same native mode-A setup already used by the Valley Arachnoth route:
        // move direction 3, mode A, both interpolator rates 60, clear m38E.
        arachnoth_startDragonEvade(0x3C);
        formation->m28_state=7;
        break;
    default:break;
    }
    engine->m3CC->m8=0;
    engine->m3CC->m0=0;
}
