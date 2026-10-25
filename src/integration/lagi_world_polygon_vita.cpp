#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_world_polygon.h"
#include "lagi/native_ray_clip.h"

// Native FLD_A7 06069AD0 transforms four world vertices, then 0606993C
// near-clips, far-rejects, projects and screen-clips the fixed four-corner
// primitive. MAIN 0602CAF4 emits its untextured VDP1 polygon packet.
void lagiDisplayWorldPolygon(const sVec3_FP* points,std::uint16_t color,std::int32_t depth)
{
    if(!points)return;
    lagi::NativeRayQuad view{};
    for(unsigned i=0;i<4;++i){
        sVec3_FP transformed;
        transformAndAddVecByCurrentMatrix(&points[i],&transformed);
        for(unsigned axis=0;axis<3;++axis)view[i][axis]=transformed[axis].asS32();
    }
    auto& clip=graphicEngineStatus.m405C;
    if(!lagi::nativeClipPolygonDepth(view,clip.m10_nearClipDistance.asS32(),
        clip.m14_farClipDistance.asS32()))return;
    lagi::NativeRayQuad projected{};
    std::array<float,4> inverseDepth{};
    for(unsigned i=0;i<4;++i){
        if(view[i][2]<=0)return; // Invalid projection state must not divide by zero.
        const fixedPoint reciprocal=FP_Div(0x10000,fixedPoint(view[i][2]));
        projected[i][0]=MTH_Mul_5_6(clip.m18_widthScale,fixedPoint(view[i][0]),reciprocal).asS32();
        projected[i][1]=MTH_Mul_5_6(clip.m1C_heightScale,fixedPoint(view[i][1]),reciprocal).asS32();
        inverseDepth[i]=65536.0f/float(view[i][2]);
    }
    if(!lagi::nativeClipRayQuad(projected,clip.m4,clip.m6,clip.m0,clip.m2,&inverseDepth))return;
    if(depth<0)depth=view[0][2];
    auto& context=graphicEngineStatus.m14_vdp1Context[0];
    if(context.mC>=1018 || context.m0_currentVdp1WriteEA==mainContextVdp1[0].end() ||
        context.m20_pCurrentVdp1Packet>=context.m24_vdp1Packets+1024)return;
    auto& command=*context.m0_currentVdp1WriteEA;command={};
    command.m0_CMDCTRL=0x1004;command.m4_CMDPMOD=0x5C0;command.m6_CMDCOLR=color;
    command.mC_CMDXA=static_cast<s16>(projected[0][0]);command.mE_CMDYA=static_cast<s16>(-projected[0][1]);
    command.m10_CMDXB=static_cast<s16>(projected[1][0]);command.m12_CMDYB=static_cast<s16>(-projected[1][1]);
    command.m14_CMDXC=static_cast<s16>(projected[2][0]);command.m16_CMDYC=static_cast<s16>(-projected[2][1]);
    command.m18_CMDXD=static_cast<s16>(projected[3][0]);command.m1A_CMDYD=static_cast<s16>(-projected[3][1]);
    context.m20_pCurrentVdp1Packet->m4_bucketTypes=static_cast<u16>((
        static_cast<u32>(depth)*static_cast<u32>(clip.m38_oneOverFarClip.asS32()))>>16);
    context.m20_pCurrentVdp1Packet->m6_vdp1EA=&command;
    ++context.m20_pCurrentVdp1Packet;++context.m0_currentVdp1WriteEA;++context.m1C;++context.mC;
    const float depths[4]={1.f/inverseDepth[0],1.f/inverseDepth[1],1.f/inverseDepth[2],1.f/inverseDepth[3]};
    lagiRecordVdp1WorldDepth(&command,depths);
}
