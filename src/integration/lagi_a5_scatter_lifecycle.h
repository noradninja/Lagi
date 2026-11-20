#pragma once

// Disc 1 BTL_A5 0605E936..E9DA. State 6 is an escape, not a kill:
// after the native 0x5A counter, retire the actor and count it at the
// formation. Do NOT award XP/Dyne or mark the life meter defeated.
template <class Enemy>
inline bool lagiA5ScatterRetireIfComplete(Enemy* enemy)
{
    if (enemy->m28_state != 6 || enemy->getTask()->isFinished())
        return false;
    const auto old = enemy->m24_counter++;
    if (old <= 0x5A)
        return false;
    enemy->m24_counter = 0;
    ++enemy->m8_formation->mE0_entityGroup.mF_deadCount;
    enemy->getTask()->markFinished();
    return true;
}
