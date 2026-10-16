#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "kernel/fade.h"
#include "kernel/moduleManager.h"

bool lagiAzelCommitMapWorldConfirmation(const LagiAzelMapConfirmationState& state)
{
    if(!state.finished || state.running || !state.result) return false;
    // Native0601798C calls setNextGameStatus(0x4F) before touching the
    // optional gameplay-state backup at0604FD88 (m40E4).
    setNextGameStatus(0x4F);
    if(graphicEngineStatus.m40E4) {
        auto& saved=graphicEngineStatus.m40E4->m2B0;
        // Original offsets+2B0/+2D4 are the two channels of sFadeControls.
        fadePalette(&saved.m0_fade0,0,0,1);
        fadePalette(&saved.m24_fade1,0,0,1);
    }
    // Native0604FD52 is menu state0604FD50+2, not a task finish flag.
    graphicEngineStatus.m40AC.m2=1;
    return true;
}
