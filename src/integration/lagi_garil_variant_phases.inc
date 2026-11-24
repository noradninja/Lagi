    case 3:
        if(battleEngine_isPlayerTurnActive())return;
        if(pThis->mF1_pad5[1])return; // native formation +F2 reaction latch
        if(pEngine->m3CC->m8==1)lagiSelectGarilVariantTurn(pThis);
        break;
    case 4:
        if(lagiGarilVariantCounterAfter(pThis->m24_counter1,15)) {
            pThis->m24_counter1=0;
            pEngine->m188_flags.m100_attackAnimationFinished=1;pThis->m28_state=5;
        }
        break;
    case 5:
        if(!pEngine->m188_flags.m400000) {
            pThis->m28_state=3;pEngine->m3CC->m8=0;pEngine->m3CC->m0=0;
        }
        break;
    case 6:
        if(pEngine->m188_flags.m2000) {
            battleEngine_displayAttackName(15,30,0);
            lagiTransitionFormationActors(pThis->mE0_entityGroup,0x04000000u,
                0x04000000u,0x02000000u,lagiGarilVariantActorFlags);
            pThis->m28_state=7;
        }
        break;
    case 7:
        if(lagiGarilVariantCounterAfter(pThis->m24_counter1,105)) {
            pThis->m24_counter1=0;
            pEngine->m188_flags.m100_attackAnimationFinished=1;pThis->m28_state=5;
        }
        break;
    case 8:
        if(lagiGarilVariantCounterAfter(pThis->m26_counter2,75)) {
            pThis->m26_counter2=0;decreaseNPCRefCount(0xE);pThis->getTask()->markFinished();
        }
        break;
    default:
        pThis->m28_state=8;
        break;
