#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/garil_script_choice.h"
#include "field/field_a5/o_fld_a5.h"
#include "kernel/dialogTask.h"

LagiGarilChoiceStep lagiAzelRunGarilChoice30(s_fieldScriptWorkArea* script,sSaturnPtr& pc)
{
    if (!gFLD_A5 || pc.m_file!=gFLD_A5) return LagiGarilChoiceStep::Unsupported;
    // A5 06070B4C..BA2: operand is signed and keeps its sign in the
    // dialog constructor (unlike opcode 31). Yield at the opcode while busy.
    const auto opcode=pc-1;
    const s8 count=readSaturnS8(pc);
    if (script->m3C_multichoiceTask) {
        if (script->m3C_multichoiceTask->m0_Status!=4) {
            pc=opcode;return LagiGarilChoiceStep::Yield;
        }
        pc=pc+1;
        if (count>0) {
            pc=pc+3;pc.m_offset&=~3u;pc=pc+count*4;
        }
        return LagiGarilChoiceStep::Yield;
    }
    pc=pc+1;
    if (!count) return LagiGarilChoiceStep::Continue;
    auto choices=pc+3;choices.m_offset&=~3u;
    startDialogTask(script,&script->m3C_multichoiceTask,&script->m54_currentResult,count,choices);
    pc=opcode;
    return LagiGarilChoiceStep::Yield;
}
