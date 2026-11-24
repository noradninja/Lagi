    case 2:
        if(pEngine->m188_flags.m2000) {
            battleEngine_displayAttackName(1,30,0);
            lagiTransitionFormationActors(pThis->mE0_entityGroup,0x04000000u,
                0x04000000u,0x02000000u,lagiA5ActorFlags);
            pThis->m28_state=3;
        }
        break;
    case 3:
        if(lagiGarilDualTimerAfter(pThis->m24_counter1,15)) {
            pThis->m24_counter1=0;pEngine->m188_flags.m100_attackAnimationFinished=1;
            pThis->m28_state=1;
        }
        break;
    case 4:
        if(pEngine->m188_flags.m2000) {
            battleEngine_displayAttackName(2,35,0);
            lagiTransitionFormationActors(pThis->mE0_entityGroup,0x04000000u,
                0x04000000u,0x02000000u,lagiA5ActorFlags);
            pThis->m28_state=5;
        }
        break;
    case 5:
        if(lagiGarilDualTimerAfter(pThis->m24_counter1,60)) {
            pThis->m24_counter1=0;
            pThis->mE0_entityGroup.m8_dataTable=g_BTL_A5->getSaturnPtr(
                0x060B11C8+u8(pThis->m27C_variant*0x78));
            pEngine->m188_flags.m100_attackAnimationFinished=1;pThis->m28_state=1;
        }
        break;
