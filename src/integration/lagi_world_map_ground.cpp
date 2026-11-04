#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_ground.h"
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
}
