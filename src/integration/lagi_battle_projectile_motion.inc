// BTL_A3 0605D610: the native collision arguments are position + radius,
// not plain position vectors. Keep the boundary typed, without changing the
// reconstructed gunshot callers that still need their own layout audit.
struct sLagiBattleSphere { sVec3_FP position; fixedPoint radius; };
static bool lagiBattleSpheresOverlap(const sLagiBattleSphere& a,
    const sLagiBattleSphere& b)
{
    const auto delta=b.position-a.position;
    const fixedPoint radius=a.radius+b.radius;
    return !(MTH_Mul(radius,radius)<dot3_FP(&delta,&delta));
}

// 060A473C. The order of arithmetic shift / multiply is significant for odd
// negative accelerations. Azel's yaw and COMMON sine/cosine tables are retained.
static void lagiBattleProjectileMotion(const sVec3_FP& source,
    const sVec3_FP& target,const sLagiBattleProjectileDescriptor& d,
    const sVec3_FP& scroll,sVec3_FP& velocity,sVec3_FP& acceleration)
{
    const auto delta=target-source;
    sVec2_FP angles;
    computeLookAt(delta,angles);
    acceleration[0]=MTH_Mul(d.horizontalAcceleration,getCos(angles[1].getInteger()));
    acceleration[1]=d.verticalAcceleration;
    acceleration[2]=MTH_Mul(d.horizontalAcceleration,getSin(angles[1].getInteger()));
    const auto time=fixedPoint::fromInteger(d.travel);
    for(int axis=0;axis<3;++axis){
        velocity[axis]=FP_Div(delta[axis],time)-
            fixedPoint((acceleration[axis].m_value>>1)*s32(d.travel));
        velocity[axis]+=scroll[axis];
    }
}

// BTL_A5 060541BA..06054260. Each acceleration uses the pre-integration
// position/velocity; SH arithmetic shifts and 32-bit wrap are intentional.
static void lagiBattleHomingProjectileMotion(const sVec3_FP& target,
    sVec3_FP& position,sVec3_FP& velocity,sVec3_FP& acceleration)
{
    for(int axis=0;axis<3;++axis){
        const s32 delta=s32(u32(target[axis].m_value)-u32(position[axis].m_value));
        acceleration[axis].m_value=s32(u32(delta>>7)-u32(velocity[axis].m_value>>3));
    }
    for(int axis=0;axis<3;++axis){
        velocity[axis].m_value=s32(u32(velocity[axis].m_value)+u32(acceleration[axis].m_value));
        position[axis].m_value=s32(u32(position[axis].m_value)+u32(velocity[axis].m_value));
    }
}

// Shared native 06064400/060644B0: view-space spheres, with the contact
// midpoint transformed through the camera's inverse matrix. Native integer
// division rounds a negative odd sum toward zero, unlike an arithmetic shift.
static bool lagiBattleSphereImpact(const sLagiBattleSphere& a,
    const sLagiBattleSphere& b,sVec3_FP& worldImpact)
{
    if(!lagiBattleSpheresOverlap(a,b))return false;
    sVec3_FP midpoint;
    for(int axis=0;axis<3;++axis)
        midpoint[axis].m_value=s32(u32(a.position[axis].m_value)+u32(b.position[axis].m_value))/2;
    transformAndAddVec(midpoint,worldImpact,cameraProperties2.m28[0]);
    return true;
}
