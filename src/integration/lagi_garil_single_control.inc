void battleEngine_PlayAttackCamera(int);
void battleEngine_displayAttackName(int,int,int);

static bool lagiGarilSingleDead(const sLagiGarilSingleFormation* formation,int index)
{
    s32 dead;
    std::memcpy(&dead,formation->mF4_entityPositions+index*sizeof(dead),sizeof(dead));
    return dead!=0;
}
static bool lagiGarilSingleLive(sLagiGarilSingleFormation* formation,int index)
{
    return lagiVisitFormationActor(formation->mE0_entityGroup,index,[](p_workArea){});
}
u32& lagiGarilSingleActorFlags(p_workArea actor)
{
    return static_cast<sBTL_A5_SingleEnemy*>(actor)->m14_flags;
}

// Original 0605AF6C: cycle 0 -> 1 -> default -> 0. The writable tables
// survive layout switches within the encounter, as the original RAM does.
static void lagiSelectGarilSingleLayout(sLagiGarilSingleFormation* formation)
{
    const unsigned old=formation->mF1_pad5[0]++;
    formation->m_lagiLayout=old==0 ? 1 : old==1 ? 2 : 0;
    if(old>1)formation->mF1_pad5[0]=0;
    formation->mE0_entityGroup.m8_dataTable=g_BTL_A5->getSaturnPtr(lagi::garilSinglePositionTable(old));
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    engine->m45C_perQuadrantDragonSpeed[0]=0x31000;
    engine->m45C_perQuadrantDragonSpeed[1]=old<2 ? 0x2E000 : 0x30000;
    engine->m45C_perQuadrantDragonSpeed[2]=old==1 ? 0x20000 : 0x2A000;
    engine->m45C_perQuadrantDragonSpeed[3]=old<2 ? 0x2E000 : 0x30000;
    engine->m364_perQuadrantDragonAltitude={0x2C000,old<2 ? 0x2D000 : 0x27000,
        0x3F000,old==0 ? 0x26000 : old==1 ? 0x2D000 : 0x27000};
}
static void lagiInitGarilSingleLayouts(sLagiGarilSingleFormation* formation)
{
    for(unsigned layout=0;layout<3;++layout)
        for(unsigned i=0;i<6;++i)
            formation->m_lagiLayouts[layout][i]=readSaturnVec3(g_BTL_A5->getSaturnPtr(0x060AF90C+layout*0x48+i*12));
    formation->mF0_flag=0;
    formation->mF1_pad5[0]=randomNumber()&3;
    formation->mF1_pad5[1]=formation->mF1_pad5[2]=0;
    std::memset(formation->mF4_entityPositions,0,sizeof(formation->mF4_entityPositions));
    lagiSelectGarilSingleLayout(formation);
}

// Original 0605B558: ordered pair updates, including leader self-jitter.
// Keep all three layout tables writable without mutating the overlay assets.
static void lagiUpdateGarilSingleSpacing(sLagiGarilSingleFormation* formation)
{
    if(formation->mF1_pad5[1]>0x3F){
        formation->mF1_pad5[1]&=0x3F;
        lagiSelectGarilSingleLayout(formation);
    }
    auto& positions=formation->m_lagiLayouts[formation->m_lagiLayout];
    const int count=formation->mE0_entityGroup.mE_numEntities;
    for(int i=0;i<count;++i){
        if(lagiGarilSingleDead(formation,i) || !lagiGarilSingleLive(formation,i))continue;
        for(int j=0;j<count;++j){
            if(lagiGarilSingleDead(formation,j) || !lagiGarilSingleLive(formation,j))continue;
            auto& other=positions[j];
            if(i==j){
                if(j<2){other[0]+=fixedPoint((randomNumber()&0x1FFF)-0x1000);
                    other[2]+=fixedPoint((randomNumber()&0x1FFF)-0x1000);}
            }else{
                const auto delta=positions[i]-other;
                const auto distance=vecDistance(positions[i],other);
                if(distance.m_value<0xA000){
                    other[0]+=fixedPoint(delta[0].m_value<0 ? 0x1000 : -0x1000);
                    other[2]+=fixedPoint(delta[2].m_value<0 ? 0x1000 : -0x1000);
                }else if(distance.m_value<0x32000){
                    other[0]+=fixedPoint((randomNumber()&0x1FFF)-0x1000);
                    other[2]+=fixedPoint((randomNumber()&0x1FFF)-0x1000);
                }else{
                    other[0]+=fixedPoint(delta[0].m_value<0 ? -0x2000 : 0x2000);
                    other[2]+=fixedPoint(delta[2].m_value<0 ? -0x4000 : 0x4000);
                }
            }
            if(other[0].m_value>=0x19000)other[0]=0x19000;
            if(other[0].m_value<=-0x19000)other[0]=-0x19000;
            if(other[2].m_value>=0x14000)other[2]=0x14000;
            if(other[2].m_value<=-0x14000)other[2]=-0x14000;
        }
    }
}

// Original 0605B83C. Movement-only turns retain the native signed-byte
// postincrement and quadrant-specific policy; no guessed random selection.
static void lagiSelectGarilSingleAttack(sLagiGarilSingleFormation* formation)
{
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    const int quadrant=engine->m22C_dragonCurrentQuadrant;
    if(quadrant==1 || quadrant==3){
        if(!formation->mF1_pad5[2]){
            battleEngine_initiateEnemyMoveDragon(quadrant==1 ? 1 : 2,0x3C);
            formation->m28_state=4;
        }
        return;
    }
    if(quadrant==2){
        if(!formation->mF1_pad5[2]){
            const s8 old=s8(formation->m_lagiMoveCounter++);
            if(old!=0){battleEngine_initiateEnemyMoveDragon(3,0x3C);formation->m_lagiMoveCounter=0;}
            formation->m28_state=4;
        }
        return;
    }
    if(quadrant!=0)return;
    int begin=0,end=2;
    bool scan=true,resetCounter=false;
    if(gBattleManager->m10_battleOverlay->m18_dragon->m1C0_statusModifiers&8){
        if(lagiGarilSingleDead(formation,2) && lagiGarilSingleDead(formation,3) &&
            lagiGarilSingleDead(formation,4) && lagiGarilSingleDead(formation,5)){
            const s8 old=s8(formation->m_lagiMoveCounter++);
            if(old==0 || old==1){
                battleEngine_initiateEnemyMoveDragon(old==0 ? 1 : 2,0x3C);
                formation->m28_state=4;scan=false;
            }else if(old==2){formation->m28_state=4;scan=false;}
            else resetCounter=true;
        }else{begin=2;end=formation->mE0_entityGroup.mE_numEntities;}
    }else if(lagiGarilSingleDead(formation,0) && lagiGarilSingleDead(formation,1)){
        begin=2;end=formation->mE0_entityGroup.mE_numEntities;
    }else resetCounter=true;
    bool eligible=false;
    if(scan)for(int i=begin;i<end;++i)
        if(!lagiGarilSingleDead(formation,i) && lagiGarilSingleLive(formation,i) &&
            !(formation->mF1_pad5[2]&(1u<<i)))eligible=true;
    if(resetCounter)formation->m_lagiMoveCounter=0;
    if(eligible){
        formation->m_lagiAttackLead=begin==0;
        for(int i=begin;i<end;++i)
            if(!lagiGarilSingleDead(formation,i))
                lagiVisitFormationActor(formation->mE0_entityGroup,i,[](p_workArea actor){
                    lagiGarilSingleActorFlags(actor)|=0x0C000000u;
                });
        battleEngine_PlayAttackCamera(1);
        battleEngine_FlagQuadrantBitForAttack(1);
        formation->m28_state=5;
    }else{
        formation->m28_state=3;
        engine->m3CC->m8=0;
        engine->m3CC->m0=0;
    }
}
