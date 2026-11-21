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

// Native entity-group +0xC is the enemy record ID. Group +8 is a position
// table, not a descriptor whose +0xC contains an ID (A5 0605E2B2..E2BA,
// 0605BC3E..BC46). Keep this typed for expanded host/Vita group layouts.
template<class EntityGroup>
inline int lagiBattleEnemyType(const EntityGroup& group, int variant)
{
    return group.mC_formationNameIndex + variant;
}

// Leaving combat without defeat must detach dragon-owned meters before the
// actor storage retires. Do not set defeated/reward bits for fleeing actors.
template<class LifeMeter>
inline void lagiRetireBattleEnemyMeter(LifeMeter*& meter)
{
    if(meter){meter->getTask()->markFinished();meter=nullptr;}
}
