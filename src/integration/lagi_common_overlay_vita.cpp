#include <cstdio>
#include <vector>

#include "lagi/lagi_compat.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"
#include "commonOverlay.h"
#include "audio/soundDataTable.h"

sCommonOverlay_data* gCommonFile = nullptr;

static std::vector<u8> g_commonDataStorage;

void initCommonFile()
{
    assert(gCommonFile == nullptr);
    new sCommonOverlay_data;
}

sCommonOverlay_data::sCommonOverlay_data()
{
    m_name = "COMMON.DAT";
    m_base = 0x00200000;

    if (!lagi::disc::read_file("COMMON.DAT", g_commonDataStorage)) {
        lagi::platform::logging::writef("[Common] failed to load COMMON.DAT\n");
        return;
    }

    m_data = g_commonDataStorage.data();
    m_dataSize = static_cast<u32>(g_commonDataStorage.size());
    gCommonFile = this;

    // Exact Azel commonOverlay.cpp layout: dragonLevelStats.
    sSaturnPtr pDataTable = getSaturnPtr(0x00206FF8);
    for (int i = 0; i < 9; ++i) {
        sSaturnPtr pData = readSaturnEA(pDataTable + 4 * i);
        sDragonLevelStat entry{};

        for (int j = 0; j < 3; ++j) entry.m0[j]  = readSaturnS8(pData + 0x00 + j);
        for (int j = 0; j < 3; ++j) entry.m3[j]  = readSaturnS8(pData + 0x03 + j);
        for (int j = 0; j < 3; ++j) entry.m6[j]  = readSaturnS8(pData + 0x06 + j);
        for (int j = 0; j < 3; ++j) entry.m9[j]  = readSaturnS8(pData + 0x09 + j);
        for (int j = 0; j < 3; ++j) entry.mC[j]  = readSaturnS8(pData + 0x0C + j);
        for (int j = 0; j < 3; ++j) entry.mF[j]  = readSaturnS8(pData + 0x0F + j);
        for (int j = 0; j < 3; ++j) entry.m12[j] = readSaturnS8(pData + 0x12 + j);
        for (int j = 0; j < 3; ++j) entry.m15[j] = readSaturnS8(pData + 0x15 + j);
        for (int j = 0; j < 3; ++j) entry.m18[j] = readSaturnS8(pData + 0x18 + j);
        for (int j = 0; j < 3; ++j) entry.m1B[j] = readSaturnS8(pData + 0x1B + j);

        dragonLevelStats.push_back(entry);
    }

    soundDataTableInit();

    // Exact Azel commonOverlay.cpp layout: battle overlay descriptors.
    sSaturnPtr battleOverlaySetupEA = getSaturnPtr(0x002005DC);
    for (int i = 0; i < 27; ++i) {
        sBattleOverlaySetup entry{};
        entry.m0_name = readSaturnString(readSaturnEA(battleOverlaySetupEA + 0x00));
        entry.m4_prg  = readSaturnString(readSaturnEA(battleOverlaySetupEA + 0x04));
        entry.m8_fnt  = readSaturnString(readSaturnEA(battleOverlaySetupEA + 0x08));
        entry.mC_numSubBattles = readSaturnU32(battleOverlaySetupEA + 0x0C);

        sSaturnPtr subBattlesEA = readSaturnEA(battleOverlaySetupEA + 0x10);
        for (u32 j = 0; j < entry.mC_numSubBattles; ++j) {
            entry.m10_subBattles.push_back(readSaturnString(readSaturnEA(subBattlesEA)));
            subBattlesEA += 4;
        }

        battleOverlaySetup.push_back(entry);
        battleOverlaySetupEA += 0x14;
    }

    // Exact Azel commonOverlay.cpp layout: battle activation bytes.
    sSaturnPtr battleActivationListEA = getSaturnPtr(0x002002BC);
    for (int i = 0; i < 27; ++i)
        battleActivationList.push_back(readSaturnS8(battleActivationListEA + i));

    lagi::platform::logging::writef("[Common] loaded %u bytes: %u dragon stats, %u sound configs, %u battle descriptors, %u activation entries\n",
                m_dataSize,
                static_cast<unsigned>(dragonLevelStats.size()),
                static_cast<unsigned>(SoundDataTable.size()),
                static_cast<unsigned>(battleOverlaySetup.size()),
                static_cast<unsigned>(battleActivationList.size()));
}
