#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_panels_draw.h"
#include "kernel/vdp1Allocator.h"

namespace lagi::worldmap {
void draw_map_panels(MapPanels& s,sSaturnPtr program,s_workAreaCopy* owner,bool ordinaryMap)
{
    if(program.isNull() || !owner || !owner->m4_vd1Allocation) return;
    // WORLD 0605738C. Camera state 1 supplies slide=96, selected=-1 and
    // ordinary/fleet bit 0. Keep the original offscreen motion, not a fade.
    s.slide=ordinaryMap?std::max(s.slide-2,0):std::min(s.slide+2,96);
    auto& context=graphicEngineStatus.m14_vdp1Context[0];
    for(unsigned panel=0;panel<2;++panel) {
        if(map_panel_variant(mainGameState.bitField,panel)<0) continue;
        int frame=0;
        if(s.selected==int(panel)) {
            s.frame=s16(u16(s.frame)+u16(s16(s.direction)));
            if(s.frame>=11) {s.frame=1;s.direction=1;}
            frame=s.frame;
        }
        if(frame<0 || frame>10) continue;
        const auto sources=readSaturnEA(program+0x5BE8+panel*4);
        const auto placement=program+0x5BF4+panel*8;
        auto& cmd=*context.m0_currentVdp1WriteEA;
        cmd.m0_CMDCTRL=0x1000;cmd.m4_CMDPMOD=0x00A0;
        cmd.m6_CMDCOLR=u16(0x2200+panel*0x100);
        cmd.m8_CMDSRCA=u16(owner->m4_vd1Allocation->m4_vdp1Memory+readSaturnU16(sources+frame*2));
        cmd.mA_CMDSIZE=readSaturnU16(program+0x5BF0+panel*2);
        cmd.mC_CMDXA=s16(u32(s32(readSaturnS16(placement+2)))-(panel?u32(s.slide):0u));
        cmd.mE_CMDYA=s16(0u-u32(s32(readSaturnS16(placement+6)))-(panel?0u:u32(s.slide)));
        context.m20_pCurrentVdp1Packet->m4_bucketTypes=u16(MTH_Mul(
            0x199,graphicEngineStatus.m405C.m38_oneOverFarClip).asS32());
        context.m20_pCurrentVdp1Packet->m6_vdp1EA=&cmd;
        ++context.m20_pCurrentVdp1Packet;++context.m1C;
        ++context.m0_currentVdp1WriteEA;++context.mC;
    }
    if(s.selected<0) {s.frame=0;s.direction=1;}
}
}
