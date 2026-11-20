#include "lagi/lagi_world_polygon.h"

// FLD_A7 06069F58 and 0606A400: 16 segments, angle step01000000,
// alternating bottom slots0/1 and top slots3/2. Local Y extent is m20.
static void lagiBeamSparkDrawRing(sBeamSpark* spark,bool rotated)
{
    sMatrix4x3 rotation;
    if(rotated){
        initMatrixToIdentity(&rotation);
        rotateMatrixShiftedY(fixedPoint(static_cast<s32>(spark->m10)),&rotation);
        rotateMatrixShiftedX(fixedPoint(static_cast<s32>(spark->mC)),&rotation);
    }
    std::array<sVec3_FP,4> quad;
    const auto writePoint=[&](const sVec3_FP& local,sVec3_FP& out){
        if(rotated)transformAndAddVec(local,out,rotation);
        else out=local;
        for(unsigned axis=0;axis<3;++axis)
            out[axis]=fixedPoint(static_cast<s32>(static_cast<u32>(out[axis].asS32())+
                static_cast<u32>(spark->m0_pos[axis].asS32())));
    };
    const auto edge=[&](unsigned angle,unsigned bottom){
        const fixedPoint radius(static_cast<s32>(spark->m1C));
        sVec3_FP local(MTH_Mul(radius,getCos(angle)),0,MTH_Mul(radius,getSin(angle)));
        writePoint(local,quad[bottom]);
        local[1]=fixedPoint(static_cast<s32>(spark->m20));
        writePoint(local,quad[3-bottom]);
    };
    edge(0,0);
    for(unsigned segment=1;segment<=16;++segment){
        edge((segment<<8)&0xFFF,segment&1);
        lagiDisplayWorldPolygon(quad.data(),spark->m2C,-1);
    }
}

// Historical upstream name: this is the unrotated DRAW callback, not Update.
void beamSpark_UpdateAlt(sBeamSpark* spark)
{
    lagiBeamSparkDrawRing(spark,false);
}

void beamSpark_Draw(sBeamSpark* spark)
{
    lagiBeamSparkDrawRing(spark,true);
}
