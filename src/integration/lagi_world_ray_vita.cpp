#include "lagi/lagi_azel_upstream_prelude.h"
#include "kernel/rayDisplay.h"
#include "lagi/native_ray_clip.h"

static s32 lagiWorldRayClipDepth(std::array<sVec3_FP,2>& view,s_graphicEngineStatus_405C& clip)
{
    std::array<std::array<std::int32_t,3>,2> line{};
    for(unsigned i=0;i<2;++i)for(unsigned axis=0;axis<3;++axis)line[i][axis]=view[i][axis].asS32();
    if(!lagi::nativeClipRayDepth(line,clip.m10_nearClipDistance.asS32(),clip.m14_farClipDistance.asS32()))return 0;
    for(unsigned i=0;i<2;++i)for(unsigned axis=0;axis<3;++axis)view[i][axis]=fixedPoint(line[i][axis]);
    return 1;
}
// Exact shared projection body with only its visibility call redirected to
// the original mutating near/far clip. Existing battle-ray behavior is intact.
#include "lagi_world_ray_projection.inc"

// MAIN 0602E136..0602E1A8, unshaded world ray. Even source zero remains a
// native distorted-sprite command; there is no desktop solid-line fallback.
void displayRaySegmentFromWorldSpace(sVec3_FP* points,s32 width,u16 source,
    u16 size,u16 color,u16 mode)
{
    if(!points)return;
    std::array<sVec3_FP,2> view;
    transformAndAddVecByCurrentMatrix(&points[0],&view[0]);
    transformAndAddVecByCurrentMatrix(&points[1],&view[1]);
    sScreenQuad3 screen{};
    auto& clip=graphicEngineStatus.m405C;
    if(!lagiWorldRayProjection(view,width,clip,screen))return;
    lagi::NativeRayQuad projected{};
    for(unsigned i=0;i<4;++i)for(unsigned axis=0;axis<2;++axis)projected[i][axis]=screen[i][axis];
    const float a=float(view[0][2].asS32())/65536.0f,b=float(view[1][2].asS32())/65536.0f;
    std::array<float,4> inverseDepth{1.0f/a,1.0f/b,1.0f/b,1.0f/a};
    if(!lagi::nativeClipSceneQuad(projected,{clip.mC,clip.mE,clip.m8,clip.mA},
        {clip.m4,clip.m6,clip.m0,clip.m2},&inverseDepth))return;
    auto& context=graphicEngineStatus.m14_vdp1Context[0];
    if(context.mC>=1018 || context.m0_currentVdp1WriteEA==mainContextVdp1[0].end() ||
        context.m20_pCurrentVdp1Packet>=context.m24_vdp1Packets+1024)return;
    auto& command=*context.m0_currentVdp1WriteEA;command={};
    command.m0_CMDCTRL=0x1002;command.m4_CMDPMOD=0x480|mode;
    command.m6_CMDCOLR=color;command.m8_CMDSRCA=source;command.mA_CMDSIZE=size;
    command.mC_CMDXA=static_cast<s16>(projected[0][0]);command.mE_CMDYA=static_cast<s16>(-projected[0][1]);
    command.m10_CMDXB=static_cast<s16>(projected[1][0]);command.m12_CMDYB=static_cast<s16>(-projected[1][1]);
    command.m14_CMDXC=static_cast<s16>(projected[2][0]);command.m16_CMDYC=static_cast<s16>(-projected[2][1]);
    command.m18_CMDXD=static_cast<s16>(projected[3][0]);command.m1A_CMDYD=static_cast<s16>(-projected[3][1]);
    context.m20_pCurrentVdp1Packet->m4_bucketTypes=static_cast<u16>((
        static_cast<u32>(view[1][2].asS32())*static_cast<u32>(clip.m38_oneOverFarClip.asS32()))>>16);
    context.m20_pCurrentVdp1Packet->m6_vdp1EA=&command;
    ++context.m20_pCurrentVdp1Packet;++context.m0_currentVdp1WriteEA;++context.m1C;++context.mC;
    const float depths[4]={1.0f/inverseDepth[0],1.0f/inverseDepth[1],1.0f/inverseDepth[2],1.0f/inverseDepth[3]};
    lagiRecordVdp1WorldDepth(&command,depths);
}
