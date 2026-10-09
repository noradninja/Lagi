#include "lagi/lagi_azel_upstream_prelude.h"
#include "kernel/vdp1AnimatedQuad.h"
#include "kernel/rayDisplay.h"
#include <cmath>

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
    fixedPoint scale,
    const quadColor* colors = nullptr)
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
    auto& context = graphicEngineStatus.m14_vdp1Context[0];
    if (context.mC >= 1018 || (colors && context.m10 == context.m14[0].end()))
        return 0;

    s_vdp1Command& cmd =
        *graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA;
    cmd = {};
    cmd.m0_CMDCTRL = 0x1002 | (q.m2_CMDCTRL & 0x30);
    cmd.m4_CMDPMOD = (q.m4_CMDPMOD | 0x80) & ~4u;
    if (colors) {
        auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
        cmd.m1C_CMDGRA = ctx.m10 - ctx.m14[0].begin();
        *ctx.m10++ = *colors;
        cmd.m4_CMDPMOD |= 4u;
    }
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
    const quadColor* colors)
{
    return drawQuadInternal(pThis, position, fixedPoint(0x10000), colors);
}

int vdp1DrawQuadScaled(
    sAnimatedQuad* pThis,
    sVec3_FP* position,
    fixedPoint scale)
{
    return drawQuadInternal(pThis, position, scale);
}

// Native ray service: Azel supplies view-space endpoints, width, texture and
// Gouraud colors. Emit the same VDP1 distorted sprite used by particles/UI.
bool gDirectRayRendering = true;

s32 isGunShotVisible(std::array<sVec3_FP, 2>& points, s_graphicEngineStatus_405C& clip)
{
    return points[0][2] > clip.m10_nearClipDistance &&
        points[1][2] > clip.m10_nearClipDistance &&
        (points[0][2] < clip.m14_farClipDistance || points[1][2] < clip.m14_farClipDistance);
}

void displayRaySegmentFromViewSpace(std::array<sVec3_FP, 2>& points,
    s32 width, u16 characterAddress, s16 characterSize, u16 characterColor,
    const quadColor* colors, s32 colorMode)
{
    const auto& clip = graphicEngineStatus.m405C;
    if (!isGunShotVisible(points, graphicEngineStatus.m405C)) return;
    float x[2], y[2], hw[2], hh[2];
    for (unsigned i = 0; i < 2; ++i) {
        const fixedPoint invZ = FP_Div(0x10000, points[i][2]);
        x[i] = MTH_Mul_5_6(clip.m18_widthScale, points[i][0], invZ).asS32() / 65536.0f;
        y[i] = MTH_Mul_5_6(clip.m1C_heightScale, points[i][1], invZ).asS32() / 65536.0f;
        hw[i] = MTH_Mul_5_6(clip.m18_widthScale, fixedPoint(width), invZ).asS32() / 65536.0f;
        hh[i] = MTH_Mul_5_6(clip.m1C_heightScale, fixedPoint(width), invZ).asS32() / 65536.0f;
    }
    const float angle = std::atan2(y[0] - y[1], x[0] - x[1]);
    const float sn = std::sin(angle), cs = std::cos(angle);
    auto coord = [](float v) -> s16 {
        return static_cast<s16>(std::clamp(std::lround(v), -32768l, 32767l));
    };
    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    if (ctx.mC >= 1018 || ctx.m10 == ctx.m14[0].end()) return;
    auto& cmd = *ctx.m0_currentVdp1WriteEA;
    cmd = {};
    cmd.m0_CMDCTRL = 0x1002;
    cmd.m4_CMDPMOD = 0x480 | colorMode;
    cmd.m6_CMDCOLR = characterColor;
    cmd.m8_CMDSRCA = characterAddress;
    cmd.mA_CMDSIZE = characterSize;
    cmd.mC_CMDXA = coord(x[0] - hw[0] * sn);
    cmd.mE_CMDYA = coord(-(y[0] + hh[0] * cs));
    cmd.m10_CMDXB = coord(x[1] - hw[1] * sn);
    cmd.m12_CMDYB = coord(-(y[1] + hh[1] * cs));
    cmd.m14_CMDXC = coord(x[1] + hw[1] * sn);
    cmd.m16_CMDYC = coord(-(y[1] - hh[1] * cs));
    cmd.m18_CMDXD = coord(x[0] + hw[0] * sn);
    cmd.m1A_CMDYD = coord(-(y[0] - hh[0] * cs));
    if (colors) {
        cmd.m1C_CMDGRA = ctx.m10 - ctx.m14[0].begin();
        *ctx.m10++ = *colors;
        cmd.m4_CMDPMOD |= 4u;
    }
    ctx.m20_pCurrentVdp1Packet->m4_bucketTypes = 0;
    ctx.m20_pCurrentVdp1Packet->m6_vdp1EA = &cmd;
    ++ctx.m20_pCurrentVdp1Packet;
    ++ctx.m1C;
    ++ctx.mC;
    ++ctx.m0_currentVdp1WriteEA;
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
