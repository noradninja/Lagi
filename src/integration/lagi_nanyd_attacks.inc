static void lagiNanydRushVelocity(sBTL_A5_Enemy* actor,bool perturb)
{
    const auto pitch=actor->m_lagiRushHeading[0].getInteger()&0xFFF;
    const auto yaw=actor->m_lagiRushHeading[1].getInteger()&0xFFF;
    auto& v=actor->m_lagiPositionVelocity;
    v[0]=MTH_Mul(-0x4A12,getCos(yaw));
    v[2]=MTH_Mul(-0x4A12,getSin(yaw));
    v[1]=MTH_Mul(0x4A12,getSin(pitch));
    if(perturb){ // Native RNG ordering is X, Z, Y.
        v[0]+=fixedPoint(s32(randomNumber()&0xFF)-128);
        v[2]+=fixedPoint(s32(randomNumber()&0xFF)-128);
        v[1]+=fixedPoint(s32(randomNumber()&0xFF)-128);
    }
    actor->m4C_velocityY=v[1].asS32();
}
static void lagiNanydPrepareRush(sBTL_A5_Enemy* actor)
{
    auto* dragon=gBattleManager->m10_battleOverlay->m18_dragon;
    actor->m_lagiRushTarget=&dragon->m8C;
    assert(actor->m_lagiRushTarget->m8);
    computeLookAt(*actor->m_lagiRushTarget->m8-actor->m30_position,actor->m_lagiRushHeading);
    lagiNanydRushVelocity(actor,false);
    actor->m24_counter=0;
}
static bool lagiNanydContact(sBTL_A5_Enemy* actor,s32 radius,s32 damage,s32 kind)
{
    auto* target=actor->m_lagiRushTarget;
    assert(target && target->m8);
    if(!target || !target->m8)return false;
    sVec3_FP contact;
    if(!lagiBattleContactPoint(actor->m30_position,radius,*target->m8,0x3000,contact))return false;
    playSystemSoundEffect(0x67);
    const sVec3_FP zero{};
    applyDamageToDragon(*target,damage,gBattleManager->m10_battleOverlay->m18_dragon->m8_position,
        kind,zero,0x0800);
    return true;
}
// Native 0605ECF0. The mapped history point moves with the formation; the
// charge advances its existing local offset rather than spawning a projectile.
static bool lagiNanydUpdateRush(sBTL_A5_Enemy* actor)
{
    lagiNanydRushVelocity(actor,true);
    actor->m90_initialPosition+=actor->m_lagiPositionVelocity;
    auto* formation=actor->m8_formation;
    const unsigned index=readSaturnU8(formation->m1CC_dataTable2+actor->m1C_entityIndex);
    assert(index<18);
    if(index>=18)return false;
    sVec3_FP center;
    std::memcpy(&center,formation->mF4_entityPositions+index*sizeof(center),sizeof(center));
    actor->m30_position=center;
    actor->m30_position+=actor->m90_initialPosition;
    if(lagiNanydContact(actor,0x2000,actor->m17C_hitFlashCount,1))return true;
    if(actor->m24_counter++>0x45){
        actor->m24_counter=0;
        if(!formation->mF0_flag)return true;
        // Queen-loss timeout is escape retirement, never a defeat award.
        lagiRetireBattleEnemyMeter(actor->mAC_lifeMeter);
        ++formation->mE0_entityGroup.mF_deadCount;
        actor->getTask()->markFinished();
    }
    return false;
}
// Native 0605EF10: follow the coordinated formation, damage at most once,
// then finish on the old signed counter > 0x5A boundary.
static bool lagiNanydUpdateCoordinated(sBTL_A5_Enemy* actor)
{
    lagiUpdateNanydPosition(actor);
    if(!actor->m_lagiCoordinatedHit &&
        lagiNanydContact(actor,0x14000,actor->m17E_hitFlashColor,2))
        actor->m_lagiCoordinatedHit=true;
    if(actor->m24_counter++>0x5A){actor->m24_counter=0;return true;}
    return false;
}
static bool lagiUpdateNanydAttackState(sBTL_A5_Enemy* actor)
{
    switch(actor->m28_state){
    case 1:
    case 2:
        stepAnimation(&actor->m2C_models[0]);
        stepAnimation(&actor->m2C_models[0]);
        if(actor->m28_state==1){
            sVec2_FP angles;
            computeLookAt(actor->m30_position-gBattleManager->m10_battleOverlay->m18_dragon->m8_position,angles);
            actor->m84_displayRotation[0]=angles[0];actor->m84_displayRotation[1]=angles[1];actor->m84_displayRotation[2]=0;
        }
        lagiUpdateNanydPosition(actor);
        if(actor->m14_flags&0x02000000u){
            if(actor->m28_state==1){lagiNanydPrepareRush(actor);actor->m28_state=3;}
            else{
                actor->m_lagiRushTarget=&gBattleManager->m10_battleOverlay->m18_dragon->m8C;
                actor->m24_counter=0;actor->m28_state=4;
            }
        }
        return true;
    case 3:
        stepAnimation(&actor->m2C_models[0]);stepAnimation(&actor->m2C_models[0]);
        if(lagiNanydUpdateRush(actor))actor->m28_state=0;
        return true;
    case 4:
        stepAnimation(&actor->m2C_models[0]);stepAnimation(&actor->m2C_models[0]);
        if(lagiNanydUpdateCoordinated(actor)){
            actor->m28_state=0;actor->m_lagiCoordinatedHit=false;actor->m14_flags&=~0x08000000u;
        }
        return true;
    case 5:
        actor->m14_flags&=~0x40000000u;actor->m28_state=0;
        return true;
    default:return false;
    }
}
static void lagiNanydEnterPendingAttack(sBTL_A5_Enemy* actor)
{
    if(actor->m28_state==0 && (actor->m14_flags&0x04000000u))
        actor->m28_state=(actor->m14_flags&0x08000000u) ? 2 : 1;
}
