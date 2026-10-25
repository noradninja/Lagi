// BTL_T0 0605A214..0605A53A: contextual prompt retirement.
static void tutorialController_checkTextCondition(sTutorialControllerTask* pThis)
{
    auto* overlay=gBattleManager->m10_battleOverlay;
    auto* menu=overlay->m20_battleHud->m28_battleCommandMenu;
    if(!menu || !(menu->m20&8)) pThis->m20_flags&=~0x10u;
    else if(!(pThis->m20_flags&0x10)) {
        pThis->m20_flags|=0x10;tutorialController_clearTextArea();
    }
    const auto* engine=overlay->m4_battleEngine;
    const unsigned mode=static_cast<unsigned>(engine->m38C_battleMode)&0xFF;
    const int sub=static_cast<s8>(engine->m38D_battleSubMode);
    bool finished=false,clearMenuLatch=false;
    switch(pThis->m18_textSet) {
    case 0:case 1:case 2:case 3:
        finished=static_cast<u8>(engine->m22C_dragonCurrentQuadrant)==pThis->m18_textSet;
        clearMenuLatch=true;
        break;
    case 4: finished=mode==0 && sub>=3;break;
    case 5: finished=mode==3 && sub>=0;break;
    case 6: finished=mode==1 && sub>=0;break;
    case 7: finished=mode==4 && sub>=0;break;
    case 8: finished=(mode==0 && sub>=3) ||
        ((mode==1 || mode==3 || mode==4) && sub>=0);break;
    default: break;
    }
    if(finished) {
        if(clearMenuLatch) pThis->m20_flags&=~0x10u;
        pThis->m20_flags&=~8u;
        tutorialController_clearTextArea();
    }
}
