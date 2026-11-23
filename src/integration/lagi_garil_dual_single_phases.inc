    case 6:
        if(pEngine->m188_flags.m2000) {
            battleEngine_displayAttackName(8,35,0);pThis->m28_state=7;
        }
        break;
    case 7:
        if(lagiGarilDualTimerAfter(pThis->m24_counter1,45)) {
            pThis->m24_counter1=0;
            pThis->mC0_position2=gBattleManager->m10_battleOverlay->m18_dragon->mF0;
            pThis->mA8_position=pThis->m2C_posBlock.m0_position;
            pThis->mA8_position[0]=lagi::formationAdd(pThis->mA8_position[0].asS32(),0xA000);
            pThis->mA8_position[1]=lagi::formationAdd(pThis->mA8_position[1].asS32(),-0x1000);
            pThis->mA8_position[2]=lagi::formationSubtract(pThis->mA8_position[2].asS32(),0x14000);
            pThis->m280_phase=1;
            battleEngine_enableAttackCamera();
            battleEngine_setDesiredCameraPositionPointer(&pThis->mC0_position2);
            battleEngine_setCurrentCameraPositionPointer(&pThis->mA8_position);
            battleEngine_resetCameraInterpolation();
            lagiTransitionFormationActors(pThis->m1D0_entityGroup2,0x04000000u,
                0x04000000u,0x02000000u,lagiGarilSingleActorFlags);
            pThis->m28_state=8;
        }
        break;
    case 8:
        if(lagiGarilDualTimerAfter(pThis->m24_counter1,27)) {
            pThis->m24_counter1=0;pEngine->m188_flags.m100_attackAnimationFinished=1;
            battleEngine_restoreCameraDefault();pThis->m28_state=9;
        }
        break;
    case 9:
        if(!battleEngine_isPlayerTurnActive()) {
            pThis->m28_state=0;pEngine->m3CC->m8=0;pEngine->m3CC->m0=0;
            pThis->m280_phase=0;battleEngine_restoreCameraAfterEnemyAttack();
        }
        break;
