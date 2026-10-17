#include "lagi/lagi_azel_upstream_prelude.h"
#include "kernel/vdp1AnimatedQuad.h"
#include "kernel/rayDisplay.h"
#include "lagi/platform.h"
#include "lagi/lagi_particle_color_adapter.h"
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
    static unsigned missingReports = 0u, clipReports = 0u;
    static unsigned emittedReports[2]{};
    if (!pThis || !pThis->m0_quad || pThis->m0_quad->empty() || !position)
    {
        if (missingReports < 8u) {
            ++missingReports;
            lagi::platform::logging::writef("[NativeParticle] rejected=missing-data\n");
        }
        return 0;
    }

    const sVdp1Quad& q = pThis->m0_quad->at(pThis->m7_currentFrame);

    sVec3_FP viewPos;
    transformAndAddVecByCurrentMatrix(position, &viewPos);

    const s32 z = viewPos[2].asS32();
    if (z <= static_cast<s32>(graphicEngineStatus.m405C.m10_nearClipDistance) ||
        z >= static_cast<s32>(graphicEngineStatus.m405C.m14_farClipDistance))
    {
        if (clipReports < 8u) {
            ++clipReports;
            lagi::platform::logging::writef(
                "[NativeParticle] rejected=depth z=%d near=%d far=%d\n", z,
                static_cast<s32>(graphicEngineStatus.m405C.m10_nearClipDistance),
                static_cast<s32>(graphicEngineStatus.m405C.m14_farClipDistance));
        }
        return 0;
    }

    const fixedPoint invZ = FP_Div(0x10000, viewPos[2]);
    const fixedPoint scaledW = MTH_Mul(q.mC_width, scale);
    const fixedPoint scaledH = MTH_Mul(q.m10_height, scale);

    // initVDP1Projection stores integer-pixel scales in the raw fixedPoint
    // payload. These products are already pixels, not 16.16 coordinates.
    // MAIN 0602D12C..0602D186 (scaled: 0602E47A..0602E4E6) projects
    // the authored origin, then adds the independently projected dimensions.
    // The frame need not be centered on its position; offsets can animate.
    const fixedPoint originX = viewPos[0] + MTH_Mul(q.m14_X, scale);
    const fixedPoint originY = viewPos[1] + MTH_Mul(q.m18_Y, scale);
    const s32 x0 = MTH_Mul_5_6(
        graphicEngineStatus.m405C.m18_widthScale, originX, invZ).asS32();
    const s32 y0 = MTH_Mul_5_6(
        graphicEngineStatus.m405C.m1C_heightScale, originY, invZ).asS32();
    const s32 width = MTH_Mul_5_6(
        graphicEngineStatus.m405C.m18_widthScale,
        scaledW, invZ).asS32();
    const s32 height = MTH_Mul_5_6(
        graphicEngineStatus.m405C.m1C_heightScale,
        scaledH, invZ).asS32();
    const s32 x1 = x0 + width;
    const s32 y1 = y0 - height;
    const s32 cx = x0 + width / 2, cy = y0 - height / 2;
    const s32 hw = width / 2, hh = height / 2;

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

    cmd.mC_CMDXA  = clamp16(x0);
    cmd.mE_CMDYA  = clamp16(-y0);
    cmd.m10_CMDXB = clamp16(x1);
    cmd.m12_CMDYB = clamp16(-y0);
    cmd.m14_CMDXC = clamp16(x1);
    cmd.m16_CMDYC = clamp16(-y1);
    cmd.m18_CMDXD = clamp16(x0);
    cmd.m1A_CMDYD = clamp16(-y1);
    const float sceneZ = static_cast<float>(z) / 65536.0f;
    const float viewZ[4] = {sceneZ, sceneZ, sceneZ, sceneZ};
    lagiRecordVdp1WorldDepth(&cmd, viewZ);

    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    ctx.m20_pCurrentVdp1Packet->m4_bucketTypes =
        MTH_Mul(invZ, graphicEngineStatus.m405C.m38_oneOverFarClip).getInteger();
    ctx.m20_pCurrentVdp1Packet->m6_vdp1EA = &cmd;
    ++ctx.m20_pCurrentVdp1Packet;
    ++ctx.m1C;
    ++ctx.m0_currentVdp1WriteEA;
    ++ctx.mC;
    unsigned& emissionBudget = emittedReports[colors ? 1u : 0u];
    if (emissionBudget < 8u) {
        ++emissionBudget;
        lagi::platform::logging::writef(
            "[NativeParticle] emitted src=%04X size=%04X pmod=%04X "
            "z=%d center=%d,%d half=%d,%d shaded=%u gra=%u "
            "colors=%04X,%04X,%04X,%04X\n",
            static_cast<unsigned>(cmd.m8_CMDSRCA),
            static_cast<unsigned>(cmd.mA_CMDSIZE),
            static_cast<unsigned>(cmd.m4_CMDPMOD), z, cx, cy, hw, hh,
            colors ? 1u : 0u, static_cast<unsigned>(cmd.m1C_CMDGRA),
            colors ? static_cast<unsigned>((*colors)[0]) : 0x4210u,
            colors ? static_cast<unsigned>((*colors)[1]) : 0x4210u,
            colors ? static_cast<unsigned>((*colors)[2]) : 0x4210u,
            colors ? static_cast<unsigned>((*colors)[3]) : 0x4210u);
    }
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

int lagiDrawParticleWithColorTable(sAnimatedQuad* particle, sVec3_FP* position,
    sSaturnPtr colors, unsigned frameStride)
{
    if (!particle)
        return 0;
    const auto entry = colors + static_cast<unsigned>(particle->m7_currentFrame) * frameStride;
    quadColor gouraud{};
    for (unsigned corner = 0; corner < 4; ++corner)
        gouraud[corner] = readSaturnU16(entry + corner * 2u);
    return drawProjectedParticleWithGouraud(particle, position, &gouraud);
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
        x[i] = MTH_Mul_5_6(clip.m18_widthScale, points[i][0], invZ).asS32();
        y[i] = MTH_Mul_5_6(clip.m1C_heightScale, points[i][1], invZ).asS32();
        hw[i] = MTH_Mul_5_6(clip.m18_widthScale, fixedPoint(width), invZ).asS32();
        hh[i] = MTH_Mul_5_6(clip.m1C_heightScale, fixedPoint(width), invZ).asS32();
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
    const float za = static_cast<float>(points[0][2].asS32()) / 65536.0f;
    const float zb = static_cast<float>(points[1][2].asS32()) / 65536.0f;
    const float viewZ[4] = {za, zb, zb, za};
    lagiRecordVdp1WorldDepth(&cmd, viewZ);
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
    const sVec3_FP* points,
    const sBillboardSpriteParams* params)
{
    // Native 0602F610: two independently sized ends, rotated by view-space
    // direction. This is the collection radial-burst service, not a reticle.
    std::array<sVec3_FP, 2> view;
    transformAndAddVecByCurrentMatrix(&points[0], &view[0]);
    transformAndAddVecByCurrentMatrix(&points[1], &view[1]);
    const auto& clip = graphicEngineStatus.m405C;
    const auto clipEnd = [&](unsigned end, s32 plane) {
        const unsigned other = end ^ 1u;
        const s32 dz = view[end][2].asS32() - view[other][2].asS32();
        if (dz) for (unsigned axis = 0; axis < 2; ++axis)
            view[end][axis] = setDividend(view[end][axis].asS32() - view[other][axis].asS32(),
                plane - view[other][2].asS32(), dz) + view[other][axis].asS32();
        view[end][2] = plane;
    };
    const s32 nearZ = clip.m10_nearClipDistance.asS32();
    const s32 farZ = clip.m14_farClipDistance.asS32();
    if (view[0][2] < nearZ && view[1][2] < nearZ) return 2;
    if (view[0][2] < nearZ) clipEnd(0,nearZ);
    else if (view[1][2] < nearZ) clipEnd(1,nearZ);
    if (view[0][2] > farZ && view[1][2] > farZ) return 2;
    if (view[0][2] > farZ) clipEnd(0,farZ);
    else if (view[1][2] > farZ) clipEnd(1,farZ);
    const s32 angle = atan2(view[0][1].asS32() - view[1][1].asS32(),
                           view[0][0].asS32() - view[1][0].asS32()) & 0xfff;
    const fixedPoint sine = getSin(angle), cosine = getCos(angle);
    const fixedPoint a = MTH_Mul(params->m0_halfWidth, sine - cosine);
    const fixedPoint b = MTH_Mul(params->m0_halfWidth, sine + cosine);
    const fixedPoint c = MTH_Mul(params->m4_halfHeight, sine - cosine);
    const fixedPoint d = MTH_Mul(params->m4_halfHeight, sine + cosine);
    const fixedPoint inv0 = FP_Div(0x10000, view[0][2]);
    const fixedPoint inv1 = FP_Div(0x10000, view[1][2]);
    const s32 x[4] = {
        MTH_Mul_5_6(clip.m18_widthScale, view[0][0]-a, inv0).asS32(),
        MTH_Mul_5_6(clip.m18_widthScale, view[0][0]+b, inv0).asS32(),
        MTH_Mul_5_6(clip.m18_widthScale, view[1][0]+c, inv1).asS32(),
        MTH_Mul_5_6(clip.m18_widthScale, view[1][0]-d, inv1).asS32()};
    const s32 y[4] = {
        MTH_Mul_5_6(clip.m1C_heightScale, view[0][1]+b, inv0).asS32(),
        MTH_Mul_5_6(clip.m1C_heightScale, view[0][1]+a, inv0).asS32(),
        MTH_Mul_5_6(clip.m1C_heightScale, view[1][1]-d, inv1).asS32(),
        MTH_Mul_5_6(clip.m1C_heightScale, view[1][1]-c, inv1).asS32()};
    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    if (ctx.mC >= 1018 || ctx.m10 == ctx.m14[0].end()) return 2;
    auto& cmd = *ctx.m0_currentVdp1WriteEA;
    cmd = {};
    cmd.m0_CMDCTRL = 0x1002;
    cmd.m4_CMDPMOD = params->m8_CMDPMOD | 0x84;
    cmd.m6_CMDCOLR = params->mA_CMDCOLR;
    cmd.m8_CMDSRCA = params->mC_CMDSRCA;
    cmd.mA_CMDSIZE = params->mE_CMDSIZE;
    const auto coordinate = [](s32 value) {return static_cast<s16>(std::clamp<s32>(value,-32768,32767));};
    cmd.mC_CMDXA = coordinate(x[0]); cmd.mE_CMDYA = coordinate(-y[0]);
    cmd.m10_CMDXB = coordinate(x[1]); cmd.m12_CMDYB = coordinate(-y[1]);
    cmd.m14_CMDXC = coordinate(x[2]); cmd.m16_CMDYC = coordinate(-y[2]);
    cmd.m18_CMDXD = coordinate(x[3]); cmd.m1A_CMDYD = coordinate(-y[3]);
    cmd.m1C_CMDGRA = ctx.m10 - ctx.m14[0].begin();
    for (unsigned i = 0; i < 4; ++i) (*ctx.m10)[i] = params->m10_gouraud[i];
    ++ctx.m10;
    // First two corners belong to end 0; last two belong to end 1.
    const float z0 = static_cast<float>(view[0][2].asS32()) / 65536.0f;
    const float z1 = static_cast<float>(view[1][2].asS32()) / 65536.0f;
    const float viewZ[4] = {z0,z0,z1,z1};
    lagiRecordVdp1WorldDepth(&cmd,viewZ);
    ctx.m20_pCurrentVdp1Packet->m4_bucketTypes =
        fixedPoint(static_cast<s32>(static_cast<u32>(view[1][2].asS32()) *
            static_cast<u32>(clip.m38_oneOverFarClip.asS32()))).getInteger();
    ctx.m20_pCurrentVdp1Packet->m6_vdp1EA = &cmd;
    ++ctx.m20_pCurrentVdp1Packet;
    ++ctx.m1C; ++ctx.mC; ++ctx.m0_currentVdp1WriteEA;
    return 0;
}

void displayRaySegment_2Width(std::array<sVec3_FP, 2>& points,
    std::array<fixedPoint, 2>& widths, u16 characterAddress, s16 characterSize,
    u16 characterColor, const quadColor* colors, s32 colorMode)
{
    sBillboardSpriteParams params{};
    params.m0_halfWidth = widths[0]; params.m4_halfHeight = widths[1];
    params.m8_CMDPMOD = static_cast<u16>(colorMode);
    params.mA_CMDCOLR = characterColor; params.mC_CMDSRCA = characterAddress;
    params.mE_CMDSIZE = characterSize;
    for (unsigned i = 0; i < 4; ++i) params.m10_gouraud[i] = colors ? (*colors)[i] : 0x4210;
    drawImmediateBillboardSprite(points.data(), &params);
}

void flushParticleBillboards()
{
}
