// Source-faithful services omitted from the portable FLD_A3 reconstruction.
// Original 0607281E (projection), 060726DC (shape), 0606E750 (target lists).
#include "lagi/lagi_azel_upstream_prelude.h"
#include "field/fieldRadar.h"
#include "field/fieldDragon.h"
#include "lagi/lagi_radar_geometry.h"

void drawGaugeVdp1(u16 mode, std::array<sVec2_S16, 4>& quad, u16 color, fixedPoint depth);

static void drawNativeRadarMarker(const sVec3_FP& position, unsigned color, unsigned flags)
{
    auto* field = getFieldTaskPtr()->m8_pSubFieldData;
    auto* radar = field->m33C_fieldRadar;
    auto* dragon = field->m338_pDragonTask;
    if (!radar || !dragon || ((flags & 0x10) && (field->m354 & 4))) return;
    sVec3_FP delta = position - dragon->m8_pos;
    const s32 range = radar->m59 ? 0x7fffffff : radar->m64_encounterDistance.asS32();
    const s32 rangeSquared = radar->m59 ? 0x7fffffff : radar->m68_encounterDistanceSq.asS32();
    // 060728AE/C2/DA: comparisons are strict; equality is outside the radar.
    const auto abs32 = [](s32 n) { return n < 0 ? 0u - static_cast<u32>(n) : static_cast<u32>(n); };
    if (abs32(delta[0].asS32()) >= static_cast<u32>(range) ||
        abs32(delta[2].asS32()) >= static_cast<u32>(range)) return;
    const s32 horizontalSquared = static_cast<s32>(static_cast<u32>(FP_Pow2(delta[0]).asS32()) +
                                                    static_cast<u32>(FP_Pow2(delta[2]).asS32()));
    if (horizontalSquared >= rangeSquared || radar->m64_encounterDistance == 0) return;
    sMatrix4x3 rotation;
    initMatrixToIdentity(&rotation);
    rotateMatrixShiftedY(-dragon->m20_angle[1], &rotation);
    sVec3_FP rotated;
    transformAndAddVec(delta, rotated, rotation);
    // 06035686 is setDividend: scale * value / divisor, with 16.16 result.
    const s16 offsetX = static_cast<s16>(setDividend(0x0013cccc, rotated[0], radar->m64_encounterDistance) >> 16);
    const s16 offsetY = static_cast<s16>(setDividend(static_cast<s32>(0xffee0000u), rotated[2], radar->m64_encounterDistance) >> 16);
    const s16 x = static_cast<s16>(radar->mC_X + radar->m28 + offsetX);
    const s16 y = static_cast<s16>(radar->mE_Y + radar->m2A + offsetY);
    if (delta[1] > radar->m5C) flags |= 4;
    else if (delta[1] < -radar->m60) flags |= 8;
    flags &= 0xf;
    std::array<sVec2_S16, 4> quad;
    const auto nativeQuad = lagi::native_radar_marker_quad(x, y, flags);
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < 2; ++j) quad[i][j] = nativeQuad[i][j];
    static constexpr u16 colors[] = {0xc3f0, 0xc3ff, 0xfe1f, 0xfff0, 0xf39c}; // 06093F54
    if (color >= std::size(colors)) return;
    auto begin = graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA;
    drawGaugeVdp1(0xc0, quad, colors[color], 0);
    lagiMarkFieldRadarCommands(&*begin, &*graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA);
}

void lagiDrawFieldRadarObjectMarkers(s_LCSTask* task)
{
    // Both pending and active lists are visited in the original; do not filter
    // item types, availability, or colors using renderer-side guesses.
    const auto draw = [](sLCSTarget* target) {
        if (!target || !target->m8_LCSWorldCoordinates) return;
        sVec3_FP position = *target->m8_LCSWorldCoordinates;
        if (target->m10_flags & sLCSTarget::e_locationIsWorld)
            transformAndAddVec(*target->m8_LCSWorldCoordinates, position, cameraProperties2.m28[0]);
        drawNativeRadarMarker(position, target->m10_flags & 3, 1);
    };
    for (s32 i = 0; i < task->mC; ++i) draw(task->m14[i].m0);
    for (auto* active = task->m828_activeStart; active; active = active->m4_next) draw(active->m8);
}

void lagiDrawFieldRadarDestination(s_FieldRadar* radar, const s32* position)
{
    // 06071EC4: horizontal bearing uses the native computeLookAt path after
    // shifting the X/Z delta by four; the Y component is deliberately zero.
    auto* dragon = getFieldTaskPtr()->m8_pSubFieldData->m338_pDragonTask;
    sVec3_FP destination = {position[0], position[1], position[2]};
    sVec3_FP delta = {(destination[0].asS32() - dragon->m8_pos[0].asS32()) >> 4, 0,
                     (destination[2].asS32() - dragon->m8_pos[2].asS32()) >> 4};
    sVec2_FP look{};
    computeLookAt(delta, look);
    u32 wrapped = static_cast<u32>(dragon->m20_angle[1].asS32() - look[1].asS32()) & 0x0fffffffu;
    if (wrapped & 0x08000000u) wrapped |= 0xf0000000u;
    const s32 angle = static_cast<s32>(wrapped);
    if (angle >= -0x02000000 && angle <= 0x02000000) {
        const u32 index = static_cast<u32>(angle >> 16) & 0xfff;
        const s32 sine = getSin(index).asS32();
        const s32 cosine = getCos(index).asS32();
        const s16 x = radar->mC_X + radar->m28;
        const s16 y = radar->mE_Y + radar->m2A;
        auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
        auto begin = ctx.m0_currentVdp1WriteEA;
        auto& cmd = *ctx.m0_currentVdp1WriteEA;
        cmd.m0_CMDCTRL = 0x1006;
        cmd.m4_CMDPMOD = 0x4c0;
        cmd.m6_CMDCOLR = 0xf39c;
        cmd.mC_CMDXA = static_cast<s16>(x + ((radar->m54 * sine) >> 16));
        cmd.mE_CMDYA = -static_cast<s16>(y + ((radar->m54 * cosine) >> 16));
        cmd.m10_CMDXB = static_cast<s16>(x + ((radar->m50 * sine) >> 16));
        cmd.m12_CMDYB = -static_cast<s16>(y + ((radar->m50 * cosine) >> 16));
        ctx.m20_pCurrentVdp1Packet->m4_bucketTypes = 0;
        ctx.m20_pCurrentVdp1Packet->m6_vdp1EA = &cmd;
        ++ctx.m20_pCurrentVdp1Packet;
        ++ctx.m1C;
        ++ctx.m0_currentVdp1WriteEA;
        ++ctx.mC;
        lagiMarkFieldRadarCommands(&*begin, &*ctx.m0_currentVdp1WriteEA);
    }
    drawNativeRadarMarker(destination, 4, 0x12);
}
