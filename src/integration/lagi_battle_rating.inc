// BTL_A3 0606EA44..0606EBA4: thresholds remain in the original overlay.
// m190[0] is counted in battle updates; m190[1] is damage received.
#include "lagi_battle_records.inc"
static s32 lagiBattleRatingBand(s32 value,sSaturnPtr thresholds)
{
    for(s32 band=0;band<4;++band)
        if(value<readSaturnS16(thresholds+band*2))return band;
    return 4;
}
eBattleRating battleResultScreen_computeBattleRating(sBattleResultScreen*)
{
    const auto table=readSaturnEA(g_BTL_GenericData->getSaturnPtr(0x060AB010)
        +gBattleManager->m4_previousBattleOverlayId*4);
    s32 score=8;
    if(!table.isNull()) {
        const auto row=table+gBattleManager->m8_previousSubBattleId*16;
        const auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
        score=lagiBattleRatingBand(performDivision(30,engine->m190[0]),row)
            +lagiBattleRatingBand(engine->m190[1],row+8);
    }
    const s32 rating=score==0?4:score==1?3:score==2?2:score>=7?0:1;
    lagiRecordBattleResult(rating);
    return static_cast<eBattleRating>(rating);
}
