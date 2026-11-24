// BTL_A5_3 060543F4..060544E4: native turn-ready, reaction countdown and
// quadrant-based attack selection. 06050224 is controller port2, not story data.
void battleEngine_displayAttackName(int,int,int);
static void lagiSelectLathumTurn(LagiLathumRig& rig,s_battleEngine& engine) {
    if(!engine.m3CC->m8 || battleEngine_isPlayerTurnActive())return;
    engine.m3CC->m8=0;engine.m3CC->m0=0;
    battleEngine_SetBattleMode(m7);
    if(rig.groupCounter>0 && --rig.groupCounter==0) {
        const auto buttons=graphicEngineStatus.m4514.m0_inputDevices[1].m0_current.m6_buttonDown;
        battleEngine_displayAttackName((buttons&4)?11:3,30,0);
        rig.mode=3;rig.substate=0;return;
    }
    for(unsigned index=0;index<rig.groups.size();++index) {
        const auto quadrant=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3EDD+index));
        if(quadrant==engine.m22C_dragonCurrentQuadrant && rig.groups[index].state) {
            battleEngine_displayAttackName(2,30,0);
            rig.mode=2;rig.substate=0;return;
        }
    }
    battleEngine_displayAttackName(1,30,0);rig.mode=1;rig.substate=0;
}
