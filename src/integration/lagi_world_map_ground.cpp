#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_ground.h"
#include "lagi/world_map_panels.h"
#include "3dModels.h"
#include "kernel/fileBundle.h"

namespace lagi::worldmap {
bool draw_map_ground_base(sSaturnPtr program,s_workAreaCopy* owner)
{
    if(program.isNull() || !owner || !owner->m0_fileBundle) return false;
    // Native task Init 06055740 clears translation and all three rotations.
    // Its base Draw inherits the live camera, then uses this authored list.
    const auto entries=readSaturnEA(program+0x481C);
    const s32 count=readSaturnS32(program+0x4820);
    if(count<0 || (count && entries.isNull())) return false;
    pushCurrentMatrix();
    for(s32 i=0;i<count;++i) {
        const auto entry=entries+i*4;
        LCSItemBox_DrawType0Sub0(owner->m0_fileBundle,
            readSaturnS16(entry),readSaturnS16(entry+2));
    }
    popMatrix();
    return true;
}

bool draw_map_ground(MapGround& state,sSaturnPtr program,s_workAreaCopy* owner)
{
    if(!draw_map_ground_base(program,owner)) return false;
    if(state.selected<0) return true;
    // Retail selector (060558F6 tail-call to 06055772). Pad mode 7's
    // keyboard-only sequence preview is not a Vita gameplay control.
    const int variant=map_panel_variant(mainGameState.bitField,unsigned(state.selected));
    if(variant<0) return true;
    const auto selection=program+0x4B24+state.selected*8;
    const auto variants=readSaturnEA(selection);
    const auto poses=readSaturnEA(selection+4);
    if(variants.isNull() || poses.isNull()) return false;
    const auto descriptor=variants+variant*8;
    const auto sequence=readSaturnEA(descriptor);
    const int frames=readSaturnU8(descriptor+4);
    if(sequence.isNull() || state.frame<0 || state.frame>=frames) return false;
    const auto pair=poses+readSaturnU8(sequence+state.frame)*4;
    pushCurrentMatrix();
    const sVec3_FP offset={0,0x5000,0};
    translateCurrentMatrix(offset);
    LCSItemBox_DrawType0Sub0(owner->m0_fileBundle,
        readSaturnS16(pair),readSaturnS16(pair+2));
    popMatrix();
    if(state.frame<frames-1) ++state.frame;
    return true;
}
}
