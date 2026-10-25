#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "field.h"

// MAIN06012434 is setFieldSubTaskVar0: its caller06011BB2 passes1
// in s_FieldSubTaskWorkArea::Init, immediately after storing field+8 and
// clearing subfield+370. Read the original live byte, not inventory state.
extern u8 fieldSubTaskVar0;

bool lagiAzelResolveMapParentSource(LagiAzelMapParentSource& source)
{
    source={};
    LagiAzelMapParentSource result;
    result.standalone=graphicEngineStatus.m40AC.m0_menuId!=1;
    const int scene=mainGameState.readPackedBits(0,2);
    if(scene==2) {
        if(!lagiAzelResolveTownMapAvailability(result.availability)) return false;
        source=result;
        return true;
    }
    LagiAzelMapAvailabilityInputs input;
    input.sceneClass=scene;
    input.worldMapStoryBit=mainGameState.getBit(193);
    result.hasContext=lagiAzelResolveCurrentMapContext(result.context);
    input.hasFieldContext=result.hasContext;
    if(scene==0 && input.worldMapStoryBit && fieldSubTaskVar0!=0) {
        auto* field=getFieldTaskPtr();
        if(!field || !gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000)
            return false;
        if(!lagiAzelReadFieldWorldMapRoute(gCommonFile->m_data,gCommonFile->m_dataSize,
            field->m2C_currentFieldIndex,field->m2E_currentSubFieldIndex,
            fieldSubTaskVar0!=0,input.worldMapStoryBit,input.fieldRoute)) return false;
    }
    result.availability=lagiAzelMapAvailabilityFromNative(input);
    source=result;
    return true;
}

bool lagiAzelMapWorldDispatchAvailable()
{
    // MAIN0601798C requests status4F; the module-manager status table routes
    // that to mode6 (WORLD.PRG), not mode5 (CHANGE.PRG). This checks dispatch
    // presence only: the existing mode6 boundary is not yet a playable map.
    extern s_workArea*(*overlayDispatchTable[])(s_workArea*,std::int32_t);
    return overlayDispatchTable[6]!=nullptr;
}
