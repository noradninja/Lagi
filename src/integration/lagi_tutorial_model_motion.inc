// BTL_T0 06058996: move before animation/damage. The parent formation's
// position/rotation and authored target fields stay distinct from actor state.
void lagiUpdateTutorialModelMotion(const LagiBattleModelMotionView& m)
{
    for (unsigned axis=0;axis<3;++axis) {
        lagi::NativeFormationSpringAxis p{m.targetPosition[axis].asS32(),
            m.currentPosition[axis].asS32(),m.velocity[axis].asS32(),
            m.acceleration[axis].asS32(),m.worldPosition[axis].asS32()};
        lagi::stepNativeFormationInterpolation(p,m.parentPosition[axis].asS32(),m.interpolationRate);
        m.currentPosition[axis].m_value=p.current;
        m.velocity[axis].m_value=p.velocity;
        m.worldPosition[axis].m_value=p.world;
    }
    for (unsigned axis=0;axis<3;++axis) {
        lagi::NativeFormationSpringAxis r{m.targetRotation[axis].asS32(),
            m.currentRotation[axis].asS32(),m.angularVelocity[axis].asS32(),
            m.angularAcceleration[axis].asS32(),m.worldRotation[axis].asS32()};
        lagi::stepNativeFormationAngularSpring(r,m.parentRotation[axis].asS32(),0xff,0xfff);
        m.currentRotation[axis].m_value=r.current;
        m.angularVelocity[axis].m_value=r.velocity;
        m.angularAcceleration[axis].m_value=r.acceleration;
        m.worldRotation[axis].m_value=r.world;
    }
}
