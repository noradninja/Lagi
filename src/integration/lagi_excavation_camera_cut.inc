// Saturn reference: the authored Excavation entrance camera cuts directly to
// black, without presenting the normal flight camera restored by UpdateSub1.
// Request at camera completion, not the later module-manager Draw callback.
// Equal black endpoints take effect immediately in fadePalette; its native
// completion flags still provide the existing teardown barrier. No extra tick.
static void lagiCutCompletedExcavationCamera()
{
    if (gGameStatus.m0_gameMode != 3 || gGameStatus.m4_gameStatus != 0x50 ||
        getFieldTaskPtr()->m2C_currentFieldIndex != 1 ||
        getFieldTaskPtr()->m2E_currentSubFieldIndex != 0 ||
        !getFieldSpecificData_A3()->m9C_A3_1_exits[2] ||
        mainGameState.getBit(0xA * 8 + 6))
        return;
    fadePalette(&g_fadeControls.m0_fade0, 0, 0, 1);
    fadePalette(&g_fadeControls.m24_fade1, 0, 0, 1);
}
