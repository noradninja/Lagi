// Native A5 0605EFEC..F09C; m170..178 are a destination offset, not a
// guessed per-frame velocity. Position/rotation use the shared native springs.
void lagiSignalNanydScatter(sEntityGroup* group)
{
    lagiSignalBattleFormation(*group,3,[](p_workArea actor)->u32& {
        return static_cast<sBTL_A5_Enemy*>(actor)->m14_flags;
    });
}
// Native 0605EAA4 shares the same positional/angular springs as other
// model actors. Keep authored offsets separate from the current spring state.
static void lagiUpdateNanydModelMotion(sBTL_A5_Enemy* actor)
{
    auto& group=actor->m8_formation->mE0_entityGroup;
    actor->m60_targetPosition=readSaturnVec3(group.m8_dataTable+actor->m1C_entityIndex*12)
        +actor->m_lagiScatterOffset+actor->m198_orbitVelocity;
    const auto history=readSaturnU8(actor->m8_formation->m1CC_dataTable2+actor->m1C_entityIndex);
    assert(history<actor->m8_formation->m_lagiNanydHistory.size());
    const sVec3_FP parentRotation={}; // Native formation position-block +0xC (zero).
    LagiBattleModelMotionView motion{actor->m30_position,actor->m90_initialPosition,
        actor->m60_targetPosition,actor->m_lagiScatterVelocity,actor->m_lagiScatterAcceleration,
        actor->m3C_rotation,actor->m9C_targetRotation,actor->m84_displayRotation,
        actor->m_lagiScatterAngularVelocity,actor->m_lagiScatterAngularAcceleration,
        actor->m8_formation->m_lagiNanydHistory[history],parentRotation,0};
    lagiUpdateBattleModelSpringMotion(motion);
}
static bool lagiUpdateNanydScatterActor(sBTL_A5_Enemy* actor)
{
    if (actor->m28_state!=0 && actor->m28_state!=6) return false;
    auto* dragon=gBattleManager->m10_battleOverlay->m18_dragon;
    if (actor->m28_state==0) {
        if (!(actor->m14_flags&8)) return false;
        actor->m_lagiScatterOffset=actor->m30_position-dragon->mF0;
        actor->m_lagiScatterOffset.m4_Y+=s32(performModulo2(0x28000,randomNumber()))-0x14000;
        if (actor->m_lagiScatterOffset.m4_Y<0)
            actor->m_lagiScatterOffset.m4_Y=-actor->m_lagiScatterOffset.m4_Y;
        for (int axis=0;axis<3;++axis)
            actor->m_lagiScatterOffset[axis]=MTH_Mul(0x60000,actor->m_lagiScatterOffset[axis]);
        BTL_A5_enemy_cleanup(actor,nullptr);
        actor->m28_state=6;
        // Native enters state 6 on the following update.
        return true;
    }
    stepAnimation(&actor->m2C_models[0]);
    stepAnimation(&actor->m2C_models[0]);
    auto& group=actor->m8_formation->mE0_entityGroup;
    lagiUpdateNanydModelMotion(actor);
    sVec3_FP look=dragon->mF0-actor->m30_position;
    sVec2_FP angles;
    computeLookAt(look,angles);
    actor->m84_displayRotation.m0_X=angles[0];
    actor->m84_displayRotation.m4_Y=angles[1];
    actor->m84_displayRotation.m8_Z=0;
    if (actor->m24_counter++>0x5A) {
        actor->m24_counter=0;
        ++group.mF_deadCount;
        actor->getTask()->markFinished();
    }
    return true;
}
