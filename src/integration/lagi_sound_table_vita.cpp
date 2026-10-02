#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"
#include "commonOverlay.h"
#include "audio/soundDataTable.h"

std::vector<sSequenceConfig> SoundDataTable;

void soundDataTableInit()
{
    SoundDataTable.clear();
    sSaturnPtr dataTableEA = gCommonFile->getSaturnPtr(0x002152D8);

    for (int i = 0; i < 79; ++i) {
        sSequenceConfig entry{};
        entry.m0 = readSaturnEA(dataTableEA);
        entry.m4_soundConfigs = readSaturnEA(dataTableEA + 4);
        entry.m8_areaMapIndex = readSaturnS16(dataTableEA + 8);
        entry.mA = readSaturnS16(dataTableEA + 0xA);
        entry.mC_numMapEntries = readSaturnU8(dataTableEA + 0xC);
        entry.mD_playerSoundTypes = readSaturnU8(dataTableEA + 0xD);
        entry.mE = readSaturnU8(dataTableEA + 0xE);
        entry.mF = readSaturnU8(dataTableEA + 0xF);
        SoundDataTable.push_back(entry);
        dataTableEA += 0x10;
    }
}
