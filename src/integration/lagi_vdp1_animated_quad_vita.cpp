#include "lagi/lagi_azel_upstream_prelude.h"
#include "kernel/vdp1AnimatedQuad.h"

std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr ptr)
{
    std::vector<sVdp1Quad> value;
    while (true) {
        auto& q = value.emplace_back();
        q.m0_isLast = readSaturnU8(ptr + 0x00);
        q.m1 = readSaturnU8(ptr + 0x01);
        q.m2_CMDCTRL = readSaturnU16(ptr + 0x02);
        q.m4_CMDPMOD = readSaturnU16(ptr + 0x04);
        q.m6_CMDSRCA = readSaturnU16(ptr + 0x06);
        q.m8_CMDSIZE = readSaturnU16(ptr + 0x08);
        q.mA_CMDCOLR = readSaturnU16(ptr + 0x0A);
        q.mC_width = readSaturnFP(ptr + 0x0C);
        q.m10_height = readSaturnFP(ptr + 0x10);
        q.m14_X = readSaturnFP(ptr + 0x14);
        q.m18_Y = readSaturnFP(ptr + 0x18);
        if (q.m0_isLast == 1)
            break;
        ptr += 0x1C;
    }
    return value;
}

void particleInitSub(
    sAnimatedQuad* pThis,
    u16 vdp1Memory,
    const std::vector<sVdp1Quad>* quads)
{
    pThis->m6 = 0;
    pThis->m7_currentFrame = 0;
    pThis->m0_quad = quads;
    pThis->m4_vdp1Memory = vdp1Memory;
}

int sGunShotTask_UpdateSub4(sAnimatedQuad* pThis)
{
    if (!pThis || !pThis->m0_quad || pThis->m0_quad->empty())
        return 0;
    const sVdp1Quad& q = pThis->m0_quad->at(pThis->m7_currentFrame);
    int result = 0;
    if (q.m1 <= pThis->m6++) {
        pThis->m6 = 0;
        result = 1;
        if (q.m0_isLast == 1) {
            pThis->m7_currentFrame = 0;
            result = 3;
        } else {
            ++pThis->m7_currentFrame;
            if (pThis->m7_currentFrame >= pThis->m0_quad->size())
                pThis->m7_currentFrame = 0;
        }
    }
    return result;
}

static int drawQuadInternal(
    sAnimatedQuad* pThis,
    sVec3_FP* position,
    fixedPoint scale)
{
    if (!pThis || !pThis->m0_quad || pThis->m0_quad->empty() || !position)
        return 0;

    const sVdp1Quad& q = pThis->m0_quad->at(pThis->m7_currentFrame);

    sVec3_FP viewPos;
    transformAndAddVecByCurrentMatrix(position, &viewPos);

    const s32 z = viewPos[2].asS32();
    if (z <= static_cast<s32>(graphicEngineStatus.m405C.m10_nearClipDistance) ||
        z >= static_cast<s32>(graphicEngineStatus.m405C.m14_farClipDistance))
        return 0;

    const fixedPoint invZ = FP_Div(0x10000, viewPos[2]);
    const fixedPoint scaledW = MTH_Mul(q.mC_width, scale);
    const fixedPoint scaledH = MTH_Mul(q.m10_height, scale);

    const s32 cx = MTH_Mul_5_6(
        graphicEngineStatus.m405C.m18_widthScale, viewPos[0], invZ).getInteger();
    const s32 cy = MTH_Mul_5_6(
        graphicEngineStatus.m405C.m1C_heightScale, viewPos[1], invZ).getInteger();
    const s32 hw = std::max<s32>(1, MTH_Mul_5_6(
        graphicEngineStatus.m405C.m18_widthScale,
        scaledW / 2, invZ).getInteger());
    const s32 hh = std::max<s32>(1, MTH_Mul_5_6(
        graphicEngineStatus.m405C.m1C_heightScale,
        scaledH / 2, invZ).getInteger());

    auto clamp16 = [](s32 v) -> s16 {
        return static_cast<s16>(std::clamp<s32>(v, -32768, 32767));
    };

    s_vdp1Command& cmd =
        *graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA;
    cmd.m0_CMDCTRL = 0x1002;
    cmd.m4_CMDPMOD = q.m4_CMDPMOD | 0x84;
    cmd.m6_CMDCOLR =
        (q.m4_CMDPMOD & 0x38) == 8
            ? static_cast<u16>(pThis->m4_vdp1Memory + q.mA_CMDCOLR)
            : q.mA_CMDCOLR;
    cmd.m8_CMDSRCA =
        static_cast<u16>(pThis->m4_vdp1Memory + q.m6_CMDSRCA);
    cmd.mA_CMDSIZE = q.m8_CMDSIZE;

    cmd.mC_CMDXA  = clamp16(cx - hw);
    cmd.mE_CMDYA  = clamp16(-(cy + hh));
    cmd.m10_CMDXB = clamp16(cx + hw);
    cmd.m12_CMDYB = clamp16(-(cy + hh));
    cmd.m14_CMDXC = clamp16(cx + hw);
    cmd.m16_CMDYC = clamp16(-(cy - hh));
    cmd.m18_CMDXD = clamp16(cx - hw);
    cmd.m1A_CMDYD = clamp16(-(cy - hh));

    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    ctx.m20_pCurrentVdp1Packet->m4_bucketTypes =
        MTH_Mul(invZ, graphicEngineStatus.m405C.m38_oneOverFarClip).getInteger();
    ctx.m20_pCurrentVdp1Packet->m6_vdp1EA = &cmd;
    ++ctx.m20_pCurrentVdp1Packet;
    ++ctx.m1C;
    ++ctx.m0_currentVdp1WriteEA;
    ++ctx.mC;
    return 1;
}

int drawProjectedParticle(sAnimatedQuad* pThis, sVec3_FP* position)
{
    return drawQuadInternal(pThis, position, fixedPoint(0x10000));
}

int drawProjectedParticleWithGouraud(
    sAnimatedQuad* pThis,
    sVec3_FP* position,
    const quadColor*)
{
    return drawProjectedParticle(pThis, position);
}

int vdp1DrawQuadScaled(
    sAnimatedQuad* pThis,
    sVec3_FP* position,
    fixedPoint scale)
{
    return drawQuadInternal(pThis, position, scale);
}

int drawImmediateBillboardSprite(
    const sVec3_FP*,
    const sBillboardSpriteParams*)
{
    return 0;
}

void flushParticleBillboards()
{
}
