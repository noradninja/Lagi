// Native A5 formation state 9 writes signal 0x8 to the surviving
// enemy WORK AREA +0x14 (0605D0D4..D0E0), not the task header.
// This typed receiver preserves that distinction for the shared
// formation iteration; future overlays can supply their own receiver.
void lagiA5SignalActorFlags(p_workArea actor, u32 flags)
{
    auto* enemy = static_cast<sBTL_A5_Enemy*>(actor);
    enemy->m14_flags |= flags;
}
