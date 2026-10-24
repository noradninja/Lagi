// BTL_A3 0606F308..0606F4C6: table-authored proficiency requirements.
static void lagiUnlockBattleBerserks(sBattleResultScreen* result)
{
    for(auto& list:result->m12_learnedBerserks)list.fill(0);
    result->m7A_learnedCounts.fill(0);
    for(int category=3;category>=0;--category) {
        const auto table=readSaturnEA(g_BTL_GenericData->getSaturnPtr(0x060AC094)+category*4);
        for(unsigned entry=0;;++entry) {
            const auto row=table+entry*8;
            const s16 id=readSaturnU16(row+6);
            if(!id)break;
            bool eligible=true;
            for(unsigned i=1;i<5;++i)
                if(int(mainGameState.gameStats.m4_growthLevels[i])+1<int(readSaturnU8(row+i)))eligible=false;
            const auto item=static_cast<eItems>(id);
            if(!eligible || mainGameState.getItemCount(item))continue;
            mainGameState.setItemCount(item,1);
            auto& count=result->m7A_learnedCounts[category];
            assert(count<result->m12_learnedBerserks[category].size());
            result->m12_learnedBerserks[category][count++]=id;
            PDS_Log("[BattleBerserk] class=%d item=%d count=%u\n",category,id,unsigned(count));
        }
    }
}

// 0606F270: ascending class order, at most four names on this result page.
static void lagiPrintLearnedBattleBerserks(sBattleResultScreen* result)
{
    unsigned printed=0;
    for(unsigned category=0;category<6 && printed<4;++category)
        for(unsigned i=0;i<result->m7A_learnedCounts[category] && printed<4;++i) {
            const auto id=result->m12_learnedBerserks[category][i];
            if(!id)continue;
            const auto config=g_BTL_GenericData->getSaturnPtr(0x060ABAD0+printed*16);
            const s32 x=readSaturnS32(config),y=readSaturnS32(config+4);
            displayObjectIcon(0,x,y,getObjectIcon(static_cast<eItems>(id)));
            vdp2PrintStatus.m14_oldPalette=vdp2PrintStatus.m10_palette;
            vdp2PrintStatus.m10_palette=(readSaturnS32(config+8)&15)<<12;
            vdp2DebugPrintSetPosition(x+2,y);
            vdp2PrintfLargeFont("%s",getObjectListEntry(static_cast<eItems>(id))->m4_name.c_str());
            vdp2PrintStatus.m10_palette=vdp2PrintStatus.m14_oldPalette;
            ++printed;
        }
    if(printed)battleResultScreenProgressiveText_printString(g_BTL_GenericData->getSaturnPtr(0x060ABA70));
}
