#include "lagi/lagi_azel_upstream_prelude.h"
#include "kernel/rayDisplay.h"

// The fixed-point projection body is copied mechanically at configure time
// from the pinned native rayDisplay.cpp. No BGFX/immediate-float branch.
s32 isGunShotVisible(std::array<sVec3_FP,2>&,s_graphicEngineStatus_405C&);
#include "lagi_battle_ray_projection.inc"

void sendRaySegmentToVdp1(sScreenQuad3& screen,fixedPoint& depth,
    u16 address,s16 size,u16 color,const quadColor* gouraud,s32 mode)
{
    auto& ctx=graphicEngineStatus.m14_vdp1Context[0];
    if(!gouraud || ctx.mC>=1018 || ctx.m10==ctx.m14[0].end() ||
        ctx.m20_pCurrentVdp1Packet>=ctx.m24_vdp1Packets+1024)return;
    auto& cmd=*ctx.m0_currentVdp1WriteEA;
    cmd={};
    cmd.m0_CMDCTRL=0x1002;
    cmd.m4_CMDPMOD=0x484|mode;
    cmd.m6_CMDCOLR=color;cmd.m8_CMDSRCA=address;cmd.mA_CMDSIZE=size;
    cmd.mC_CMDXA=screen[0][0];cmd.mE_CMDYA=-screen[0][1];
    cmd.m10_CMDXB=screen[1][0];cmd.m12_CMDYB=-screen[1][1];
    cmd.m14_CMDXC=screen[2][0];cmd.m16_CMDYC=-screen[2][1];
    cmd.m18_CMDXD=screen[3][0];cmd.m1A_CMDYD=-screen[3][1];
    cmd.m1C_CMDGRA=ctx.m10-ctx.m14[0].begin();
    *ctx.m10++=*gouraud;
    // Preserve the native packet bucket. Depth is renderer-side metadata,
    // never a replacement for the authored priority/sorting key.
    ctx.m20_pCurrentVdp1Packet->m4_bucketTypes=0;
    ctx.m20_pCurrentVdp1Packet->m6_vdp1EA=&cmd;
    ++ctx.m20_pCurrentVdp1Packet;++ctx.m1C;++ctx.mC;++ctx.m0_currentVdp1WriteEA;
    const float z=static_cast<float>(depth.asS32())/65536.0f;
    const float viewZ[4]={z,z,z,z};
    lagiRecordVdp1WorldDepth(&cmd,viewZ);
}

void displayRaySegment(std::array<sVec3_FP,2>& points,s32 width,
    u16 address,s16 size,u16 color,const quadColor* gouraud,s32 mode)
{
    std::array<sVec3_FP,2> view;
    transformAndAddVecByCurrentMatrix(&points[0],&view[0]);
    transformAndAddVecByCurrentMatrix(&points[1],&view[1]);
    sScreenQuad3 screen;
    if(!rayComputeDisplayMatrix_fixedWidth(view,width,graphicEngineStatus.m405C,screen))return;
    const auto command=graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA;
    sendRaySegmentToVdp1(screen,view[1][2],address,size,color,gouraud,mode);
    if(graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA!=command) {
        const float a=static_cast<float>(view[0][2].asS32())/65536.0f;
        const float b=static_cast<float>(view[1][2].asS32())/65536.0f;
        const float viewZ[4]={a,b,b,a};
        lagiRecordVdp1WorldDepth(&*command,viewZ);
    }
}

// MAIN 0602E1AA: a world-space tapered strip, distinct from the rotated
// collection/charge square service behind displayRaySegment_2Width.
void lagiDisplayWorldStrip(std::array<sVec3_FP,2>& points,std::array<fixedPoint,2>& widths,
    u16 address,s16 size,u16 color,const quadColor* gouraud,s32 mode)
{
    std::array<sVec3_FP,2> view;
    transformAndAddVecByCurrentMatrix(&points[0],&view[0]);
    transformAndAddVecByCurrentMatrix(&points[1],&view[1]);
    sScreenQuad3 screen;
    if(!rayComputeDisplayMatrix_2Width(view,widths,graphicEngineStatus.m405C,screen))return;
    const auto command=graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA;
    sendRaySegmentToVdp1(screen,view[1][2],address,size,color,gouraud,mode);
    if(graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA!=command) {
        const float a=static_cast<float>(view[0][2].asS32())/65536.0f;
        const float b=static_cast<float>(view[1][2].asS32())/65536.0f;
        const float depths[4]={a,b,b,a};
        lagiRecordVdp1WorldDepth(&*command,depths);
    }
}
