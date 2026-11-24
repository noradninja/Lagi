    case 0:
        if(battleEngine_isBattleIntroFinished()) {
            battleEngine_SetBattleMode(eBattleModes::m7);
            battleEngine_setupAttackCamera(&pThis->m2C_posBlock.m0_position,0x50000,0,0);
            pThis->m28_state=1;
        }
        break;
    case 1:
        if(pEngine->m188_flags.m2000) {
            lagiCreateNativeBattleSpeech(pThis,17,15,g_BTL_A5->getSaturnPtr(0x060AD1A4));
            battleEngine_restoreCameraAfterEnemyAttack();
            pThis->m28_state=2;
        }
        break;
    case 2:
        if(lagiGarilVariantCounterAfter(pThis->m24_counter1,84)) {
            pThis->m24_counter1=0;
            pEngine->m188_flags.m100_attackAnimationFinished=1;
            pThis->m28_state=5;
        }
        break;
