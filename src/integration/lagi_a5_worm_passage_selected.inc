// Original 060569E8..6A20: LCS selection starts the authored tunnel
// approach and records the passage visit using the shared remapped bit API.
static void lagiA5WormPassageSelected(p_workArea owner,sLCSTarget*) {
    auto* self=static_cast<sA5WormPassageEntrance*>(owner);
    self->m9C_config=2;
    mainGameState.setBit566(readSaturnS16(self->m8_dataPtr+0x2A));
}
