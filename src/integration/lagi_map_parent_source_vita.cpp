#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "field.h"

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
    if(scene==0 && input.worldMapStoryBit) {
        auto* field=getFieldTaskPtr();
        if(!field || !gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000)
            return false;
        std::uint32_t authoredRoute=0;
        if(!lagiAzelReadFieldWorldMapRoute(gCommonFile->m_data,gCommonFile->m_dataSize,
            field->m2C_currentFieldIndex,field->m2E_currentSubFieldIndex,
            true,input.worldMapStoryBit,authoredRoute)) return false;
        // If the authored route is zero, both possible values of0024B89C
        // yield zero. A nonzero route still depends on the unknown live gate:
        // do not force it on, force it off, or silently remove the world slot.
        if(authoredRoute!=0) return false;
    }
    result.availability=lagiAzelMapAvailabilityFromNative(input);
    source=result;
    return true;
}

bool lagiAzelMapWorldDispatchAvailable()
{
    // Read the actual module-manager table. The current trimmed build leaves
    // entry5 null; do not expose an enabled transition into that empty slot.
    extern s_workArea*(*overlayDispatchTable[])(s_workArea*,std::int32_t);
    return overlayDispatchTable[5]!=nullptr;
}
