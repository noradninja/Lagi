// MAIN 0601A05E..19C: native status-menu LV/EXP/NEXT presentation.
static void lagiDrawMenuGrowth()
{
    const auto& stats = mainGameState.gameStats;
    vdp2DebugPrintSetPosition(3, 36);
    vdp2PrintfLargeFont("LV %2d", static_cast<int>(stats.m0_level) + 1);
    vdp2DebugPrintSetPosition(3, 44);
    vdp2PrintfLargeFont("EXP  %8d", stats.m20_XP);
    if (stats.m0_level >= 98) return;

    const auto row = gCommonFile->getSaturnPtr(0x0020701C + stats.m0_level * 12);
    const u32 floor = readSaturnU32(row);
    const u32 next = readSaturnU32(row + 12);
    vdp2DebugPrintSetPosition(3, 46);
    vdp2PrintfLargeFont("NEXT");
    // Clamp only malformed/legacy out-of-interval saves, not game state.
    const u32 earned = stats.m20_XP > floor ? stats.m20_XP - floor : 0;
    const u32 width = next > floor
        ? static_cast<u32>(std::min<std::uint64_t>(63, std::uint64_t(earned) * 63 / (next - floor))) : 0;
    if (!width) return;

    auto& context = graphicEngineStatus.m14_vdp1Context[0];
    if (context.mC >= 1018) return;
    auto& command = *context.m0_currentVdp1WriteEA;
    command = {};
    command.m0_CMDCTRL = 0x1004;
    command.m4_CMDPMOD = 0x04C0;
    command.m6_CMDCOLR = 0x8256;
    const s16 left = -113, right = static_cast<s16>(left + width);
    const bool pal = (VDP2Regs_.m4_TVSTAT & 1) != 0;
    const s16 top = static_cast<s16>(112 - (pal ? 114 : 98));
    const s16 bottom = static_cast<s16>(112 - (pal ? 125 : 109));
    command.mC_CMDXA = left; command.mE_CMDYA = -top;
    command.m10_CMDXB = right; command.m12_CMDYB = -top;
    command.m14_CMDXC = right; command.m16_CMDYC = -bottom;
    command.m18_CMDXD = left; command.m1A_CMDYD = -bottom;
    // Native polygon submitter 0602CAF4, caller depth 1FFFFFFF.
    context.m20_pCurrentVdp1Packet->m4_bucketTypes = static_cast<u16>(
        (u32(0x1FFFFFFF) * u32(graphicEngineStatus.m405C.m38_oneOverFarClip.asS32())) >> 16);
    context.m20_pCurrentVdp1Packet->m6_vdp1EA = &command;
    ++context.m20_pCurrentVdp1Packet;
    ++context.m0_currentVdp1WriteEA;
    ++context.m1C;
    ++context.mC;
}
