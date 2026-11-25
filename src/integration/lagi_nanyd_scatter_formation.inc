// Original A5 0605CD88..CE4E and 0605D0A0..D126. The caller still owns
// intro, normal attacks and final task retirement.
static bool lagiUpdateNanydScatterFormation(sBTL_A5_FormationTask* pThis,
                                           s_battleEngine* engine)
{
    formationUpdatePosition(&pThis->m2C_posBlock,&engine->mC_battleCenter);
    pThis->m_lagiNanydHistory[0]=pThis->m2C_posBlock.m0_position;
    for (int i=17;i>0;--i) pThis->m_lagiNanydHistory[i]=pThis->m_lagiNanydHistory[i-1];
    formationApplyAutoScroll(pThis);
    pThis->mC0_position2=pThis->m2C_posBlock.m0_position;
    pThis->mC0_position2.m8_Z-=0x12000;
    if (battleEngine_isBattleIntroFinished() && !pThis->m1D4_flag && pThis->mF0_flag) {
        pThis->m1D4_flag=1;
        battleEngine_SetBattleMode(eBattleModes::m7);
        engine->m3CC->m8=0;engine->m3CC->m0=0;engine->m3CC->m2=0;engine->m3CC->m4=0;
        pThis->m28_state=9;
        return true;
    }
    if (pThis->m28_state!=9) return false;
    if (engine->m188_flags.m2000) {
        lagiSignalNanydScatter(&pThis->mE0_entityGroup);
        battleEngine_displayAttackName(3,0x28,0);
        engine->m188_flags.m10000=1;
        engine->m188_flags.m100_attackAnimationFinished=1;
        pThis->m28_state=0xB;
    }
    return true;
}
