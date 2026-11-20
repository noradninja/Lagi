// FLD_A5 06055342. The two rotations are the low words of the
// Saturn camera angles. Keep the original fixed-point operation order.
static void lagiA5HorizonUpdate(sA5EncounterConfigTask* task)
{
    const auto& camera = cameraProperties2;
    const auto& projection = graphicEngineStatus.m405C;
    const s32 pitch = atan2(camera.m0_position[1].asS32() >> 2, 0x100000);
    const s32 angle = (static_cast<s16>(camera.mC_rotation[0]) + pitch) & 0xfff;
    const fixedPoint distance = FP_Div(0x400000, getCos(pitch & 0xfff));
    const fixedPoint height = MTH_Mul(distance, getSin(angle));
    const fixedPoint depth = MTH_Mul(distance, getCos(angle));
    task->mC_horizonY = -setDividend(projection.m1C_heightScale.asS32(),
        height.asS32(), depth.asS32());

    const s32 yaw = static_cast<s16>(camera.mC_rotation[1]);
    const fixedPoint radians = MTH_Mul_5_6(yaw, 0x3243f, 0x400000);
    const fixedPoint scaled = FP_Div(radians.asS32(), 0x800);
    const s32 scroll = setDividend(projection.m18_widthScale.asS32(),
        scaled.asS32(), 0x400000);
    task->m8_scrollX = scroll % task->m18_range2 - task->m18_range2;
}

#if LAGI_NEPTUNE_RENDERER
extern "C" int lagiVdp1BackgroundHalfWidth();
#endif

// FLD_A5 06055400 and MAIN 0602D9BC.
static void lagiA5HorizonDraw(sA5EncounterConfigTask* task)
{
    if (task->m1C_active != 1)
        return;

    const auto& projection = graphicEngineStatus.m405C;
    auto& context = graphicEngineStatus.m14_vdp1Context[0];
    const s32 halfWidth = static_cast<s16>(projection.VDP1_X2) >> 1;
    const s32 width = task->m18_range2;
    s32 coverage = halfWidth;
#if LAGI_NEPTUNE_RENDERER
    const s32 renderHalfWidth = lagiVdp1BackgroundHalfWidth();
    if (renderHalfWidth > coverage)
        coverage = renderHalfWidth;
#endif
    // Extend by whole tiles so the original camera-relative phase survives.
    s32 firstX = task->m8_scrollX - halfWidth;
    while (firstX > -coverage)
        firstX -= width;
    const s32 top = -task->mC_horizonY - task->m1A_range3;
    const s32 bottom = -task->mC_horizonY;
    const s32 sortZ = projection.m14_farClipDistance.asS32() - 1;
    const auto sortKey = static_cast<u16>((static_cast<u32>(sortZ)
        * static_cast<u32>(projection.m38_oneOverFarClip.asS32())) >> 16);
    const float depth = static_cast<float>(sortZ) / projection.m14_farClipDistance.asS32();

    for (s32 x = firstX; x <= coverage && context.mC < 1018; x += width)
    {
        auto& command = *context.m0_currentVdp1WriteEA;
        command = {};
        command.m0_CMDCTRL = 0x1002;
        command.m4_CMDPMOD = 0x0400 | task->m16_range1;
        command.m6_CMDCOLR = task->m14_range0;
        command.m8_CMDSRCA = task->m10_radarOffset;
        command.mA_CMDSIZE = task->m12_colorMask;
        command.mC_CMDXA = command.m18_CMDXD = static_cast<s16>(x);
        command.m10_CMDXB = command.m14_CMDXC = static_cast<s16>(x + width);
        command.mE_CMDYA = command.m12_CMDYB = static_cast<s16>(top);
        command.m16_CMDYC = command.m1A_CMDYD = static_cast<s16>(bottom);
        createVdp1ExtendedCommand(command)->depth = depth;

        context.m20_pCurrentVdp1Packet->m4_bucketTypes = sortKey;
        context.m20_pCurrentVdp1Packet->m6_vdp1EA = &command;
        ++context.m20_pCurrentVdp1Packet;
        ++context.m1C;
        ++context.m0_currentVdp1WriteEA;
        ++context.mC;
    }
}

