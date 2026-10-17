// Included after Azel's private sEncouterTask definition. Observation only:
// no RNG, counters, story flags, danger values or encounter rules are changed.
#include <array>
static void lagiReportEncounterState(sEncouterTask* task,sSaturnPtr row,s32 reason)
{
    const auto* sub=fieldTaskPtr->m8_pSubFieldData;
    const auto* dragon=sub ? sub->m338_pDragonTask : nullptr;
    const s32 moving=dragon ? dragon->m235_dragonSpeedIndex : -1;
    const s32 rate=row.isNull() ? 0 : readSaturnS8(row);
    const s32 rowDanger=row.isNull() ? -1 : readSaturnS8(row+1);
    const std::array<s32,11> key={fieldTaskPtr->m2C_currentFieldIndex,
        fieldTaskPtr->m2E_currentSubFieldIndex,hasEncounterData,
        static_cast<s32>(fieldTaskPtr->m28_status),static_cast<s32>(task->m18.m_offset),
        static_cast<s32>(row.m_offset),rate,rowDanger,moving,
        mainGameState.consumables[56],reason};
    static bool known=false;
    static std::array<s32,11> previous{};
    if (known && key==previous)
        return;
    known=true;previous=key;
    PDS_Log("[NativeEncounter] field=%d sub=%d enabled=%d status=%08X selector=%08X row=%08X rate=%d rowDanger=%d moving=%d shard=%d reason=%d danger=%d counter=%d initial=%d reset=%d attempts=%d/%d\n",
        key[0],key[1],key[2],static_cast<u32>(key[3]),static_cast<u32>(key[4]),
        static_cast<u32>(key[5]),key[6],key[7],key[8],key[9],key[10],
        fieldTaskPtr->m34_radarDangerLevel,task->m10,task->m14,task->m16,task->mE,task->mF);
}
