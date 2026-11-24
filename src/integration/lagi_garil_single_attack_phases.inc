    case 5:
        if(pEngine->m188_flags.m2000){
            battleEngine_displayAttackName(pThis->m_lagiAttackLead ? 8 : 7,0x23,0);
            pThis->m28_state=6;
        }
        break;
    case 6:
        if(pThis->m24_counter1++>0x2D){
            pThis->m24_counter1=0;
            lagiTransitionFormationActors(pThis->mE0_entityGroup,0x04000000u,
                0x04000000u,0x02000000u,lagiGarilSingleActorFlags);
            pThis->m28_state=7;
        }
        break;
    case 7:
        if(pThis->m24_counter1++>0x3C){
            pThis->m24_counter1=0;
            pEngine->m188_flags.m100_attackAnimationFinished=1;
            pThis->m28_state=8;
        }
        break;
    case 8:
        if(!battleEngine_isPlayerTurnActive()){
            pThis->m28_state=3;
            pEngine->m3CC->m8=0;
            pEngine->m3CC->m0=0;
            battleEngine_restoreCameraAfterEnemyAttack();
        }
        break;
