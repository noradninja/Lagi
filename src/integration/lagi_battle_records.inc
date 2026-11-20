// Saved by the native load/save task as m394_battleResults. Original storage
// is 0024BCA0: one unsigned count byte and one best-rank byte per enemy type.
extern std::array<std::uint8_t,0x104> battleResults;
static void lagiRecordBattleResult(s32 rating)
{
    const auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    // Original BTL_A3 0606EBC8..0606EC8C stops at the first negative slot.
    for(const auto entry:engine->m484) {
        if(entry<0)break;
        const auto offset=static_cast<std::size_t>(entry)*2;
        assert(offset+1<battleResults.size());
        auto& count=battleResults[offset];
        auto& best=battleResults[offset+1];
        const unsigned beforeCount=count,beforeBest=best;
        if(count<255)++count;
        if(best<rating)best=static_cast<std::uint8_t>(rating);
        PDS_Log("[BattleRecord] enemy=%d count=%u->%u best=%u->%u\n",
            int(entry),beforeCount,unsigned(count),beforeBest,unsigned(best));
    }
}
