#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/garil_script_native.h"
#include "field/field_a5/o_fld_a5.h"
#include "kernel/fade.h"

bool lagiAzelTryGarilScriptAction(sSaturnPtr target, s32 argument, s32& result)
{
    if (!gFLD_A5 || target.m_file != gFLD_A5 ||
        (target.m_offset != 0x0606FA5C && target.m_offset != 0x0606FAB4))
        return false;
    // Original A5 MOV.B sign-extends both comparison operands. Do not reuse
    // an unsigned comparison from another overlay's translated fade helper.
    if (static_cast<s8>(g_fadeControls.m_4D) >= static_cast<s8>(g_fadeControls.m_4C)) {
        vdp2Controls.m20_registers[0].m112_CLOFSL = 0x10;
        vdp2Controls.m20_registers[1].m112_CLOFSL = 0x10;
        vdp2Controls.m_isDirty = true;
    }
    const s32 duration=s32(u32(argument)<<1); // native SHLL, low 32 bits
    for (auto* fade : {&g_fadeControls.m0_fade0,&g_fadeControls.m24_fade1}) {
        if (target.m_offset == 0x0606FA5C)
            fadePalette(fade,convertColorToU32ForFade(fade->m0_color),0x8000,duration);
        else
            fadePalette(fade,0x8000,static_cast<s16>(g_fadeControls.m_48),duration);
    }
    // All recovered sites use opcode 14 (ordinary action), not result polling.
    result=0;
    return true;
}
