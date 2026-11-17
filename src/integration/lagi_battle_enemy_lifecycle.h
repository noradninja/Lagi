#pragma once

// Notify Azel's existing meter task. Bit 1 awards the authored COMMON XP/Dyne
// once; bit 8 detaches its position and retires the meter. Do not award rewards
// here or create an area-specific result path. Call once on the death transition.
// Template keeps this service independent of Saturn work-area byte offsets.
template<class LifeMeter>
inline void lagiBattleEnemyDefeated(LifeMeter* meter)
{
    if (meter)
        meter->m31 |= 1 | 8;
}
