    case 5:
        if(pEngine->m188_flags.m2000){
            battleEngine_displayAttackName(1,0x1E,0);
            lagiTransitionFormationActors(pThis->mE0_entityGroup,0x04000000u,
                0x04000000u,0x02000000u,lagiA5ActorFlags);
            pThis->m28_state=6;
        }
        break;
    case 6:
        if(pThis->m24_counter1++>0xF){
            pThis->m24_counter1=0;
            pEngine->m188_flags.m100_attackAnimationFinished=1;
            pThis->m28_state=4;
        }
        break;
    case 7:
        if(pEngine->m188_flags.m2000){
            battleEngine_displayAttackName(2,0x23,0);
            battleEngine_FlagQuadrantBitForAttack(4);
            lagiTransitionFormationActors(pThis->mE0_entityGroup,0x04000000u,
                0x04000000u,0x02000000u,lagiA5ActorFlags);
            pThis->m28_state=8;
        }
        break;
    case 8:
        if(pThis->m24_counter1++>0x3C){
            pThis->m24_counter1=0;
            // Native restores this table for both 20- and 10-actor variants.
            pThis->mE0_entityGroup.m8_dataTable=g_BTL_A5->getSaturnPtr(0x060B0D24);
            pEngine->m188_flags.m100_attackAnimationFinished=1;
            pThis->m28_state=4;
        }
        break;
