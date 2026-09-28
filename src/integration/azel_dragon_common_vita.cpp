#include <array>
#include <cstdio>
#include <vector>

#include "lagi/azel_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"
#include "commonOverlay.h"
#include "dragonData.h"

sHotpointBundle* readRiderDefinitionSub(sSaturnPtr ptrEA)
{
    if (ptrEA.isNull())
        return nullptr;
    return new sHotpointBundle(ptrEA);
}

std::array<sDragonMorphDataPerLevel, DR_LEVEL_MAX> gDragonMorphDataPerLevel;

struct sDragonAnimDataSub
{
    s32 count = -1;
    sDragonAnimDataSubRanges* m_data = nullptr;
};

struct sDragonAnimData
{
    sDragonAnimDataSub* m0 = nullptr;
    std::array<std::vector<sDragonAnimDataSub>, 3> m4;
};

static std::array<sDragonAnimData, DR_ANIM_MAX> gDragonAnimData;

static bool ptrInCommon(sSaturnPtr ptr, u32 bytes = 1)
{
    if (!gCommonFile || ptr.isNull() || ptr.m_file != gCommonFile)
        return ptr.isNull();

    const u32 address = static_cast<u32>(ptr.m_offset);
    return address >= gCommonFile->m_base &&
           address + bytes >= address &&
           address + bytes <= gCommonFile->m_base + gCommonFile->m_dataSize;
}

bool loadDragonDataFromCommonVita()
{
    if (!gCommonFile)
        return false;

    // Upstream loadDragonDataFromCommon(): morph/model metadata for all 9 forms.
    {
        sSaturnPtr ptr = gCommonFile->getSaturnPtr(0x002065E8);

        for (int i = 0; i < DR_LEVEL_MAX; ++i) {
            sDragonMorphDataPerLevel& entry = gDragonMorphDataPerLevel[i];

            entry.m0_numMCBEntries = readSaturnU32(ptr + 0);
            ptr += 4;
            entry.m4_numHotPoints = readSaturnU16(ptr + 2);
            ptr += 2;
            entry.m_m6 = readSaturnU16(ptr + 2);
            ptr += 2;

            for (int j = 0; j < 7; ++j) {
                sDragonMorphModels& sub = entry.m_m8[j];

                sub.m0_modelIndex = readSaturnU16(ptr); ptr += 2;
                sub.m2_shadowModelIndex = readSaturnU16(ptr); ptr += 2;
                sub.m4_poseModelIndex = readSaturnU16(ptr); ptr += 2;
                sub.m6_textureOffset = readSaturnU16(ptr); ptr += 2;
                sub.m8_hotPoints = nullptr;

                const sSaturnPtr hotpoints = readSaturnEA(ptr);
                ptr += 4;

                if (!hotpoints.isNull()) {
                    if (!ptrInCommon(hotpoints))
                        return false;
                    sub.m8_hotPoints = readRiderDefinitionSub(hotpoints);
                }
            }
        }
    }

    // Upstream dragon animation descriptor graph.
    {
        for (int i = 0; i < DR_ANIM_MAX; ++i) {
            const sSaturnPtr rootEA =
                readSaturnEA(gCommonFile->getSaturnPtr(0x00202054 + 4 * i));

            if (!rootEA.isNull() && !ptrInCommon(rootEA, 16))
                return false;

            if (rootEA.isNull())
                continue;

            for (int j = 0; j < 4; ++j) {
                sSaturnPtr subPtr = readSaturnEA(rootEA + j * 4);

                if (!subPtr.isNull() && !ptrInCommon(subPtr, 8))
                    return false;

                if (subPtr.isNull())
                    continue;

                while (true) {
                    sDragonAnimDataSub* output = nullptr;

                    if (j == 0) {
                        gDragonAnimData[i].m0 = new sDragonAnimDataSub;
                        output = gDragonAnimData[i].m0;
                    } else {
                        gDragonAnimData[i].m4[j - 1].emplace_back();
                        output = &gDragonAnimData[i].m4[j - 1].back();
                    }

                    output->count = readSaturnS32(subPtr + 0);
                    output->m_data = nullptr;

                    if (output->count == -1)
                        break;

                    const sSaturnPtr rangesEA = readSaturnEA(subPtr + 4);
                    if (!ptrInCommon(rangesEA, 0x30))
                        return false;

                    auto* ranges = new sDragonAnimDataSubRanges;
                    ranges->m_vec0 = readSaturnVec3(rangesEA + 0x00);
                    ranges->m_vecC = readSaturnVec3(rangesEA + 0x0C);
                    ranges->m_max  = readSaturnVec3(rangesEA + 0x18);
                    ranges->m_min  = readSaturnVec3(rangesEA + 0x24);
                    output->m_data = ranges;

                    if (j == 0)
                        break;

                    subPtr += 8;
                    if (!ptrInCommon(subPtr, 8))
                        return false;
                }
            }
        }
    }

    unsigned hotpointBundles = 0;
    unsigned animLists = 0;
    for (int i = 0; i < DR_LEVEL_MAX; ++i)
        for (int j = 0; j < 7; ++j)
            if (gDragonMorphDataPerLevel[i].m_m8[j].m8_hotPoints)
                ++hotpointBundles;

    for (int i = 0; i < DR_ANIM_MAX; ++i) {
        if (gDragonAnimData[i].m0) ++animLists;
        for (int j = 0; j < 3; ++j)
            if (!gDragonAnimData[i].m4[j].empty()) ++animLists;
    }

    std::printf("[Dragon] COMMON data loaded: %u hotpoint bundles, %u animation lists\n",
                hotpointBundles, animLists);
    return true;
}


namespace lagi::azel {
bool load_dragon_common_data()
{
    return loadDragonDataFromCommonVita();
}
} // namespace lagi::azel
