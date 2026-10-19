// BTL_A3_2 0605400A: dedicated boss bank, not the field-battle initializer.
// COMMON owns all sound/control mappings. Preserve its already-playing gate.
static void lagiArachnothInitMusic(p_workArea parent)
{
    loadSoundBanks(67, 0);
    if (findSound(100) < 0) {
        gBattleManager->m10_battleOverlay->m3 = 1;
        playPCM(parent, 100);
    }
}
