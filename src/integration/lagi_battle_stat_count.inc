// BTL_T0 0606B160: selector is an unsigned byte; increment wraps at 32 bits.
// Restore selector 8 used by the tutorial death wrapper 0605D6E4 without
// changing storage layout or pretending that the still-unmapped selectors
// have been implemented. Existing attack/item callers retain their counters.
void increaseStatsCount(int statIndex)
{
    s32* counter;
    switch (static_cast<u8>(statIndex)) {
    case 0: counter=&mainGameState.gameStats.m58_itemsUsed; break;
    case 1: counter=&mainGameState.gameStats.m5C_gunShotFired; break;
    case 2: counter=&mainGameState.gameStats.m60_homingLaserFired; break;
    case 3: counter=&mainGameState.gameStats.m64; break;
    case 8: counter=&mainGameState.gameStats.m54; break;
    default: assert(0); return;
    }
    const u32 next=static_cast<u32>(*counter)+1u;
    *counter=next<=0x7fffffffu ? static_cast<s32>(next)
        : static_cast<s32>(static_cast<s64>(next)-0x100000000ll);
}
