// Native A5 formation state 9 writes signal 0x8 to the surviving
// enemy WORK AREA +0x14 (0605D0D4..D0E0), not the task header.
// This typed receiver preserves that distinction for the shared
// formation iteration; future overlays can supply their own receiver.
void lagiA5SignalActorFlags(p_workArea actor, u32 flags)
{
    auto* enemy = static_cast<sBTL_A5_Enemy*>(actor);
    enemy->m14_flags |= flags;
}

// Original A5 0605EFEC initializes the three escape offset components.
// The Saturn overlay normalizes the away-from-reference vector to 0x60000,
// with a -0x14000 vertical bias, before its ordinary position interpolation.
// The Vita battle center is the available typed world-space reference.
void lagiA5BeginScatterFlight(sBTL_A5_Enemy* enemy, s_battleEngine* engine)
{
    const s32 dx = static_cast<s32>(enemy->m30_position.m0_X) -
                   static_cast<s32>(engine->mC_battleCenter.m0_X);
    const s32 dy = static_cast<s32>(enemy->m30_position.m4_Y) -
                   static_cast<s32>(engine->mC_battleCenter.m4_Y);
    const s32 dz = static_cast<s32>(enemy->m30_position.m8_Z) -
                   static_cast<s32>(engine->mC_battleCenter.m8_Z);
    const auto direction = lagi::nativeScatterFlightOffset(dx, dy, dz);
    enemy->m170_scatterOffset.m0_X = direction[0];
    enemy->m170_scatterOffset.m4_Y = direction[1];
    enemy->m170_scatterOffset.m8_Z = direction[2];
}
