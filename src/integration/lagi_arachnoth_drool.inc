// BTL_A3_2 0605B412..464. Upstream calls this task a smoke puff, but its
// native draw is a textured strip between the two simulated world endpoints.
#include "kernel/rayDisplay.h"
static void lagiInitArachnothStrandAllocation(sSmokePuffTask* task)
{
    // 0605A774..780 loads owner slot 0, then copies its allocation into
    // task +4. The formation parent intentionally has no VRAM allocation.
    task->m4_vd1Allocation = dramAllocatorEnd[0].mC_fileBundle->m4_vd1Allocation;
}
static void smokePuff_draw(sSmokePuffTask* task)
{
    const u16 base=task->m4_vd1Allocation->m4_vdp1Memory;
    std::array<sVec3_FP,2> points={task->m28_position,task->m4C_targetPos};
    static const quadColor neutral={0x4210,0x4210,0x4210,0x4210};
    // MAIN 0602E136 has no light/Gouraud input. The shared ray service uses
    // neutral Gouraud offsets to retain its command/depth metadata path.
    // 0108 includes native mesh stipple, not replacement alpha blending.
    displayRaySegment(points,0x400,base+0xCD0,0x0510,base+0x2EEC,&neutral,0x0108);
}
