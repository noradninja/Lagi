#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "town/town.h"

bool lagiAzelResolveTownMapAvailability(LagiAzelMapAvailability& out)
{
    out={};
    if(mainGameState.readPackedBits(0,2)!=2) return false;
    LagiAzelMapAvailabilityInputs input;
    input.sceneClass=2;
    input.worldMapStoryBit=mainGameState.getBit(193);
    if(input.worldMapStoryBit) {
        // MAIN06030060 stores town/subtown to0604A284+5C/+5D;
        // loadTownPrg owns the corresponding npcData0 fields in Azel.
        input.currentTown=static_cast<std::int8_t>(npcData0.m5C);
        //060300CC(0) is signed byte COMMON2166E4[0*2], not an
        // overlay parameter or a pointer to a town task.
        if(!gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000 ||
           gCommonFile->m_dataSize<=0x166E4) return false;
        input.returnTown=static_cast<std::int8_t>(gCommonFile->m_data[0x166E4]);
    }
    // Native field-context06016C54 returns no context for scene class2.
    out=lagiAzelMapAvailabilityFromNative(input);
    return true;
}
