// Native 0605EAA4 -> shared 060A2450/060A262A. Group-relative native
// fields bind to named host members; the 18-point history is copied data,
// never a Saturn offset applied to a polymorphic work-area pointer.
static void lagiUpdateNanydPosition(sBTL_A5_Enemy* actor)
{
    auto* formation=actor->m8_formation;
    const unsigned index=readSaturnU8(formation->m1CC_dataTable2+actor->m1C_entityIndex);
    assert(index<18);
    if(index>=18)return;
    sVec3_FP center,rotation;
    static_assert(sizeof(sVec3_FP)==12);
    std::memcpy(&center,formation->mF4_entityPositions+index*sizeof(center),sizeof(center));
    std::memcpy(&rotation,formation->m2C_posBlock.mC_pad,sizeof(rotation));
    actor->m60_targetPosition=readSaturnVec3(formation->mE0_entityGroup.m8_dataTable+actor->m1C_entityIndex*12);
    actor->m60_targetPosition+=actor->m170_scatterOffset;
    actor->m60_targetPosition+=actor->m198_orbitVelocity;
    // Preserve native live velocity fields also consumed by death fall.
    actor->m_lagiPositionVelocity[1]=actor->m4C_velocityY;
    actor->m_lagiAngularVelocity[0]=actor->m6C_deathRotX;
    actor->m_lagiAngularVelocity[1]=actor->m70_deathRotY;
    for(int axis=0;axis<3;++axis){
        lagi::NativeFormationSpringAxis p{actor->m60_targetPosition[axis].asS32(),
            actor->m90_initialPosition[axis].asS32(),actor->m_lagiPositionVelocity[axis].asS32(),
            actor->m_lagiPositionAcceleration[axis].asS32(),actor->m30_position[axis].asS32()};
        lagi::stepNativeFormationSpring(p,center[axis].asS32(),0xFF,0xFFF);
        actor->m90_initialPosition[axis]=p.current;
        actor->m_lagiPositionVelocity[axis]=p.velocity;
        actor->m_lagiPositionAcceleration[axis]=p.acceleration;
        actor->m30_position[axis]=p.world;
        lagi::NativeFormationSpringAxis a{actor->m84_displayRotation[axis].asS32(),
            actor->m9C_targetRotation[axis].asS32(),actor->m_lagiAngularVelocity[axis].asS32(),
            actor->m_lagiAngularAcceleration[axis].asS32(),actor->m3C_rotation[axis].asS32()};
        lagi::stepNativeFormationAngularSpring(a,rotation[axis].asS32(),0xFF,0xFFF);
        actor->m9C_targetRotation[axis]=a.current;
        actor->m_lagiAngularVelocity[axis]=a.velocity;
        actor->m_lagiAngularAcceleration[axis]=a.acceleration;
        actor->m3C_rotation[axis]=a.world;
    }
    actor->m4C_velocityY=actor->m_lagiPositionVelocity[1].asS32();
    actor->m6C_deathRotX=actor->m_lagiAngularVelocity[0].asS32();
    actor->m70_deathRotY=actor->m_lagiAngularVelocity[1].asS32();
}
