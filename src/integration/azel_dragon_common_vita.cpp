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
#include "lagi/disc_image.h"

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


static u32 readBE32Raw(const u8* p)
{
    return (static_cast<u32>(p[0]) << 24) |
           (static_cast<u32>(p[1]) << 16) |
           (static_cast<u32>(p[2]) << 8) |
           static_cast<u32>(p[3]);
}

static bool countHierarchyNodes(const std::vector<u8>& bundle, u32 offset,
                                unsigned& count, unsigned depth)
{
    if (offset == 0)
        return true;
    if (depth > 256 || offset + 12 > bundle.size())
        return false;

    ++count;

    const u32 child = readBE32Raw(bundle.data() + offset + 4);
    const u32 sibling = readBE32Raw(bundle.data() + offset + 8);

    if (child && !countHierarchyNodes(bundle, child, count, depth + 1))
        return false;
    if (sibling && !countHierarchyNodes(bundle, sibling, count, depth + 1))
        return false;

    return count <= 512;
}


static u16 readBE16Raw(const u8* p)
{
    return static_cast<u16>((static_cast<u16>(p[0]) << 8) | p[1]);
}

struct GeometryTotals
{
    unsigned models = 0;
    unsigned vertices = 0;
    unsigned polygons = 0;
};

static bool parseModelGeometry(const std::vector<u8>& bundle, u32 modelOffset,
                               GeometryTotals& totals)
{
    if (modelOffset + 0x14 > bundle.size())
        return false;

    const u8* base = bundle.data();
    const u32 numVertices = readBE32Raw(base + modelOffset + 4);
    const u32 verticesOffset = readBE32Raw(base + modelOffset + 8);

    if (numVertices > 65535)
        return false;

    const u64 vertexBytes = static_cast<u64>(numVertices) * 6u;
    if (verticesOffset > bundle.size() ||
        vertexBytes > bundle.size() ||
        static_cast<u64>(verticesOffset) + vertexBytes > bundle.size())
        return false;

    u32 p = modelOffset + 0x0C;
    unsigned polygons = 0;

    while (true) {
        if (p + 8 > bundle.size())
            return false;

        const u16 i0 = readBE16Raw(base + p + 0);
        const u16 i1 = readBE16Raw(base + p + 2);
        const u16 i2 = readBE16Raw(base + p + 4);
        const u16 i3 = readBE16Raw(base + p + 6);

        if (i0 == 0 && i1 == 0 && i2 == 0 && i3 == 0)
            break;

        if (i0 >= numVertices || i1 >= numVertices ||
            i2 >= numVertices || i3 >= numVertices)
            return false;

        p += 8;

        // lightingControl, CMDCTRL, CMDPMOD, CMDCOLR, CMDSRCA, CMDSIZE.
        if (p + 12 > bundle.size())
            return false;

        const u16 lightingControl = readBE16Raw(base + p);
        p += 12;

        const u8 lightingMode = static_cast<u8>((lightingControl >> 8) & 3);
        switch (lightingMode) {
            case 0:
                break;
            case 1:
                // One normal (3 x s16) plus 2 bytes padding.
                if (p + 8 > bundle.size()) return false;
                p += 8;
                break;
            case 2:
                // Four vertices, each normal + RGB555 triplet: 12 bytes each.
                if (p + 48 > bundle.size()) return false;
                p += 48;
                break;
            case 3:
                // Four normals, 6 bytes each.
                if (p + 24 > bundle.size()) return false;
                p += 24;
                break;
        }

        ++polygons;
        if (polygons > 65535)
            return false;
    }

    ++totals.models;
    totals.vertices += numVertices;
    totals.polygons += polygons;
    return true;
}

static bool traverseGeometry(const std::vector<u8>& bundle, u32 nodeOffset,
                             GeometryTotals& totals, unsigned depth)
{
    if (nodeOffset == 0)
        return true;
    if (depth > 256 || nodeOffset + 12 > bundle.size())
        return false;

    const u32 modelOffset = readBE32Raw(bundle.data() + nodeOffset + 0);
    const u32 childOffset = readBE32Raw(bundle.data() + nodeOffset + 4);
    const u32 nextOffset = readBE32Raw(bundle.data() + nodeOffset + 8);

    if (modelOffset && !parseModelGeometry(bundle, modelOffset, totals))
        return false;
    if (childOffset && !traverseGeometry(bundle, childOffset, totals, depth + 1))
        return false;
    if (nextOffset && !traverseGeometry(bundle, nextOffset, totals, depth + 1))
        return false;

    return true;
}

namespace lagi::azel {

bool validate_basic_wing_hotpoints(unsigned int* out_bones, unsigned int* out_hotpoints)
{
    if (!gCommonFile)
        return false;

    std::vector<u8> mcb;
    if (!lagi::disc::read_file("DRAGON0.MCB", mcb) || mcb.size() < 16)
        return false;

    const sDragonMorphModels& base = gDragonMorphDataPerLevel[DR_LEVEL_0_BASIC_WING].m_m8[0];
    if (!base.m8_hotPoints)
        return false;

    const u32 modelIndex = base.m0_modelIndex;
    if (modelIndex + 4 > mcb.size())
        return false;

    const u32 hierarchyOffset = readBE32Raw(mcb.data() + modelIndex);
    if (hierarchyOffset == 0 || hierarchyOffset + 12 > mcb.size())
        return false;

    unsigned boneCount = 0;
    if (!countHierarchyNodes(mcb, hierarchyOffset, boneCount, 0) || boneCount == 0)
        return false;

    std::vector<s_hotpointDefinition>* decoded =
        base.m8_hotPoints->getData(static_cast<s32>(boneCount));
    if (!decoded || decoded->size() != boneCount)
        return false;

    unsigned hotpointCount = 0;
    for (const s_hotpointDefinition& def : *decoded) {
        if (def.m0.size() != def.m4_count)
            return false;
        hotpointCount += def.m4_count;
    }

    if (out_bones) *out_bones = boneCount;
    if (out_hotpoints) *out_hotpoints = hotpointCount;

    std::printf("[Dragon] DRAGON0.MCB hierarchy: %u bones, %u decoded hotpoints\n",
                boneCount, hotpointCount);
    return true;
}



bool validate_basic_wing_geometry(unsigned int* out_models,
                                  unsigned int* out_vertices,
                                  unsigned int* out_polygons)
{
    std::vector<u8> mcb;
    if (!lagi::disc::read_file("DRAGON0.MCB", mcb) || mcb.size() < 16)
        return false;

    const sDragonMorphModels& base =
        gDragonMorphDataPerLevel[DR_LEVEL_0_BASIC_WING].m_m8[0];

    const u32 modelIndex = base.m0_modelIndex;
    if (modelIndex + 4 > mcb.size())
        return false;

    const u32 hierarchyOffset = readBE32Raw(mcb.data() + modelIndex);
    if (hierarchyOffset == 0 || hierarchyOffset + 12 > mcb.size())
        return false;

    GeometryTotals totals{};
    if (!traverseGeometry(mcb, hierarchyOffset, totals, 0))
        return false;

    if (totals.models == 0 || totals.vertices == 0 || totals.polygons == 0)
        return false;

    if (out_models) *out_models = totals.models;
    if (out_vertices) *out_vertices = totals.vertices;
    if (out_polygons) *out_polygons = totals.polygons;

    std::printf("[Dragon] DRAGON0 geometry: %u models, %u vertices, %u polygons\n",
                totals.models, totals.vertices, totals.polygons);
    return true;
}

} // namespace lagi::azel
