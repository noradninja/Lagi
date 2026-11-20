#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/garil_script_native.h"
#include "field/field_a5/o_fld_a5.h"
#include "kernel/fade.h"
#include "field/fieldCutsceneTask2.h"
#include "field/fieldCutsceneTask3.h"
#include "field/fieldCutsceneTask.h"
#include "lagi/garil_placement.h"
#include "field/fieldDragon.h"

bool lagiAzelTryGarilScriptAction(sSaturnPtr target, s32& result)
{
    if (lagiAzelTryGarilPlacement(target)) {result=0;return true;}
    if (!gFLD_A5 || target.m_file != gFLD_A5 ||
        (target.m_offset != 0x060728A4 && target.m_offset != 0x060728E0 &&
         target.m_offset != 0x060728F8 && target.m_offset != 0x0607C82A &&
         target.m_offset != 0x0607C86C)) return false;
    auto* field=getFieldTaskPtr()->m8_pSubFieldData;
    if (target.m_offset == 0x0607C82A || target.m_offset == 0x0607C86C) {
        auto* dragon=field->m338_pDragonTask;
        // Native callbacks 0600AE5A/0600AE4C disable/restore a light-wing
        // child's Draw method. Its factory is unported: do not fake that case.
        if (dragon->mB8_lightWingEffect) return false;
        dragon->m249_noCollisionAndHideDragon=target.m_offset == 0x0607C82A ? 1 : 0;
    } else if (target.m_offset == 0x060728A4) {
        // Original 060728A4 guards field, child and script +5C, then calls
        // 06071A36: the common retirement and camera-restore sequence below.
        if (field) {
            auto* script=field->m34C_ptrToE;
            if (auto* child=script->m48_cutsceneTask; child && script->m5C == 0) {
                // The Saturn service reads the common first flags word. The
                // translated children have distinct C++ types: never reuse the
                // upstream unconditional cast from task 3 to task 2.
                u32 flags;
                if (auto* task=dynamic_cast<s_cutsceneTask2*>(child)) flags=task->m0;
                else if (auto* task=dynamic_cast<s_cutsceneTask3*>(child)) flags=task->m0;
                else if (auto* task=dynamic_cast<s_cutsceneTask*>(child)) flags=task->m0;
                else return false; // unrecovered owner, not successful completion
                child->getTask()->markFinished();
                script->m48_cutsceneTask=nullptr;
                script->m5C=0;
                if (!script->m78) {
                    if(flags&2) endCutsceneCameraWithRestore();
                    else endCutsceneCameraNoRestore();
                }
            }
        }
    } else {
        field->m34C_ptrToE->m5C=target.m_offset == 0x060728F8 ? 1 : 0;
    }
    result=0; // recovered opcode-14 actions; no result-polling sites
    return true;
}

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
