// The synchronous Vita overlay loader cannot present the outgoing scene after
// its task/resources are deleted. Service MAIN 06027682..060276A0's existing
// black/30-frame request before that destructive boundary instead. Azel's
// VBlank controller owns all interpolation and completion, not a host timer.
static bool lagiWaitForModuleExitFade(s_moduleManager* owner)
{
    static s_moduleManager* waitingOwner = nullptr;
    static int waitingStatus = 0;
    const int next = gGameStatus.m8_nextGameStatus;
    const int mode = gGameStatus.m0_gameMode;
    if (!next || gGameStatus.m2 != 0 || !owner->m8 ||
        (mode != 1 && mode != 2 && mode != 3 && mode != 4)) {
        waitingOwner = nullptr;
        waitingStatus = 0;
        return false;
    }
    const int destination = readSaturnS8(gCommonFile->getSaturnPtr(0x212EAC + next * 2));
    // Cinematic/movie and town overlays share this black transition. Battle
    // entry and ordinary field/subfield transitions retain their own effects.
    if (destination != 0 && destination != 1 && destination != 2) {
        waitingOwner = nullptr;
        waitingStatus = 0;
        return false;
    }
    if (waitingOwner != owner || waitingStatus != next) {
        if (g_fadeControls.m0_fade0.m20_stopped &&
            g_fadeControls.m24_fade1.m20_stopped &&
            !(convertColorToU32ForFade(g_fadeControls.m0_fade0.m0_color) & 0x7FFF) &&
            !(convertColorToU32ForFade(g_fadeControls.m24_fade1.m0_color) & 0x7FFF))
            return false; // e.g. battle victory already completed its blackout
        if (g_fadeControls.m_4D < g_fadeControls.m_4C)
            return true; // do not steal a higher-priority native fade
        // A3 Above Excavation's completed camera sequence (status50 -> 06)
        // cuts to black on Saturn. The generic pre-teardown interpolation
        // incorrectly added a second of visible outgoing camera here.
        // Keep the authored request point; publish black before freeing VRAM.
        const int steps=(mode==3 && gGameStatus.m4_gameStatus==0x50 && next==6)?1:30;
        fadePalette(&g_fadeControls.m0_fade0,
            convertColorToU32ForFade(g_fadeControls.m0_fade0.m0_color), 0, steps);
        fadePalette(&g_fadeControls.m24_fade1,
            convertColorToU32ForFade(g_fadeControls.m24_fade1.m0_color), 0, steps);
        waitingOwner = owner;
        waitingStatus = next;
        PDS_Log("[ModuleExitFade] start mode=%d next=%02X frames=%d\n", mode, next, steps);
        return true; // publish at least one frame before allowing deletion
    }
    const bool waiting = !g_fadeControls.m0_fade0.m20_stopped ||
                         !g_fadeControls.m24_fade1.m20_stopped;
    if (!waiting) PDS_Log("[ModuleExitFade] complete next=%02X; teardown permitted\n", next);
    return waiting;
}
