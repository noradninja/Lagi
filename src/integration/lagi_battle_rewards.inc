// Original BTL_A3 0606ECA2..0606EE7A. Authored table uses pre-remap IDs.
#include "lagi/native_font_palette.h"
void battleResultScreen_printItemReceived(eBattleRating rating)
{
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    const auto table=readSaturnEA(g_BTL_GenericData->getSaturnPtr(0x060AAFD8)
        +gBattleManager->m4_previousBattleOverlayId*4);
    if(!table.isNull()) {
        // Native consumes one roll even when the rank cannot earn a bonus.
        const s32 roll=performModulo2(100,randomNumber());
        const auto row=table+gBattleManager->m8_previousSubBattleId*6;
        if(rating==eBattleRating::m3_greatFight && roll<readSaturnU8(row+4))
            engine->m480[0]=readSaturnU16(row);
        else if(rating==eBattleRating::m4_excellent && roll<readSaturnU8(row+5))
            engine->m480[0]=readSaturnU16(row+2);
        PDS_Log("[BattleRewardRoll] previous=%d/%d rank=%d roll=%d chance=%u/%u item=%d\n",
            gBattleManager->m4_previousBattleOverlayId,gBattleManager->m8_previousSubBattleId,
            s32(rating),roll,readSaturnU8(row+4),readSaturnU8(row+5),engine->m480[0]);
    }
    const s16 item=engine->m480[0];
    if(item<0)return;
    battleResultScreenProgressiveText_printString(g_BTL_GenericData->getSaturnPtr(0x060ABA00));
    // Native 0606E984 draws the icon at the config's X, and name at X+2.
    const auto config=g_BTL_GenericData->getSaturnPtr(0x060ABAB0);
    const s32 x=readSaturnS32(config),y=readSaturnS32(config+4);
    displayObjectIcon(0,x,y,getObjectIcon(static_cast<eItems>(item)));
    vdp2PrintStatus.m14_oldPalette=vdp2PrintStatus.m10_palette;
    vdp2PrintStatus.m10_palette=lagi::nativeFontPalette(readSaturnS32(config+8));
    vdp2DebugPrintSetPosition(x+2,y);
    vdp2PrintfLargeFont("%s",getObjectListEntry(static_cast<eItems>(item))->m4_name.c_str());
    vdp2PrintStatus.m10_palette=vdp2PrintStatus.m14_oldPalette;
    // Native displays even when the consumable is already at its 99 cap.
    const auto object=static_cast<eItems>(item);
    const s32 count=mainGameState.getItemCount(object);
    if(count<99)mainGameState.setItemCount(object,static_cast<s8>(count+1));
    PDS_Log("[BattleReward] item=%d rank=%d count=%d->%d previous=%d/%d\n",
        item,s32(rating),count,s32(mainGameState.getItemCount(object)),
        gBattleManager->m4_previousBattleOverlayId,gBattleManager->m8_previousSubBattleId);
}
