// BTL_A3 0606EF20, 0606F002 and 0606F0A2. COMMON owns the 12-byte
// progression rows; no thresholds or game data are copied into Lagi.
static sSaturnPtr lagiGrowthRow(unsigned level)
{
    return gCommonFile->getSaturnPtr(0x0020701C + level * 12);
}
void battleResultScreen_updateSub5(int xp)
{
    if (!xp) return;
    auto& s = mainGameState.gameStats;
    const s32 y = 0x10000 + s.m1C_dragonCursorY * 16;
    const s32 x = 0x10000 + s.m1A_dragonCursorX * 16;
    const s32 weights[5] = {0, 0x20000-x, 0x20000-y, x, y};
    for (unsigned i=1;i<5;++i) {
        const u16 gain = static_cast<u16>((MTH_Mul(weights[i], static_cast<s32>(static_cast<u32>(xp) << 16)).asS32() + 0x8000) >> 16);
        s.m24_growth[i] += gain;
        s.m80_growthXP[i] += gain;
    }
}
static s32 lagiGrowthAdjustment(s32 coefficient, u32 value, u32 sum)
{
    const auto ratio = FP_Div(static_cast<s32>(value - (sum >> 2)), static_cast<s32>(sum >> 1));
    return static_cast<s16>((MTH_Mul(coefficient * 0x10000, ratio).asS32() + 0x8000) >> 16);
}
void battleResultScreen_checkLevelUp(sBattleResultScreen* result)
{
    auto& s = mainGameState.gameStats;
    result->m73_oldLevel = s.m0_level;
    result->m74 = 0;
    result->m6_gains.fill(0);
    unsigned level = s.m0_level;
    if (level >= 98) return;
    while (level < 98) {
        const auto row = lagiGrowthRow(level+1);
        if (readSaturnU32(row) > s.m20_XP) break;
        const auto& w = s.m24_growth;
        const u32 vertical = w[2] + w[4], horizontal = w[1] + w[3];
        const s32 laser = lagiGrowthAdjustment(readSaturnS8(row+10),w[2],vertical);
        const s32 hp = lagiGrowthAdjustment(readSaturnU8(row+8),w[1],horizontal);
        const s32 bp = lagiGrowthAdjustment(readSaturnS8(row+9),w[4],vertical);
        const s32 gun = lagiGrowthAdjustment(readSaturnS8(row+11),w[3],horizontal);
        result->m6_gains[0] += readSaturnS8(row+4) + hp;
        result->m6_gains[1] += readSaturnS8(row+5) + bp;
        result->m6_gains[2] += readSaturnS8(row+6);
        result->m6_gains[3] += readSaturnS8(row+7);
        result->m6_gains[4] += laser;
        result->m6_gains[5] += gun;
        s.mC_laserPower += readSaturnS8(row+6) + laser;
        s.mE_gunPower += readSaturnS8(row+7) + gun;
        ++level;
    }
    s.m0_level = static_cast<u8>(level);
    result->m74 = static_cast<s8>(level - result->m73_oldLevel);
}
static void lagiApplyBattleGrowth(sBattleResultScreen* result)
{
    auto& s = mainGameState.gameStats;
    auto& gains = result->m6_gains;
    gains[0] = std::min<s32>(gains[0], 9999-s.mB8_maxHP);
    gains[1] = std::min<s32>(gains[1], 999-s.mBA_maxBP);
    s.m12_classMaxHP += gains[0]; s.m16_classMaxBP += gains[1];
    s.m10_currentHP += gains[0]; s.m14_currentBP += gains[1];
    updateDragonStatsFromLevel();
    for (unsigned i=1;i<5;++i) s.m24_growth[i]=0;
    // Native F002: the four combat proficiencies share the COMMON threshold
    // table but consume their own cumulative growth XP, not the player XP.
    for (unsigned i=1;i<5;++i) {
        auto& level=s.m4_growthLevels[i];
        // F002 preserves these before advancing proficiency, for result UI.
        result->m75_oldGrowthLevels[i]=level;
        while(level<98 && readSaturnU32(lagiGrowthRow(level+1))<=s.m80_growthXP[i])++level;
    }
}
