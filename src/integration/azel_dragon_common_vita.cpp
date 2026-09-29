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
#include "lagi/debug_mesh.h"
#include "lagi/platform.h"
#include <cmath>

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

    lagi::platform::logging::writef("[Dragon] COMMON data loaded: %u hotpoint bundles, %u animation lists\n",
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

    lagi::platform::logging::writef("[Dragon] DRAGON0.MCB hierarchy: %u bones, %u decoded hotpoints\n",
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

    lagi::platform::logging::writef("[Dragon] DRAGON0 geometry: %u models, %u vertices, %u polygons\n",
                totals.models, totals.vertices, totals.polygons);
    return true;
}

} // namespace lagi::azel


namespace {

struct FVec3 { float x, y, z; };

struct FMat4 {
    float m[16];
};

static FMat4 matIdentity()
{
    FMat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static FMat4 matMul(const FMat4& a, const FMat4& b)
{
    FMat4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                r.m[row * 4 + col] += a.m[row * 4 + k] * b.m[k * 4 + col];
    return r;
}

static FMat4 matTranslate(float x, float y, float z)
{
    FMat4 r = matIdentity();
    r.m[3] = x; r.m[7] = y; r.m[11] = z;
    return r;
}

static FMat4 matRotX(float a)
{
    FMat4 r = matIdentity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[5] = c; r.m[6] = -s;
    r.m[9] = s; r.m[10] = c;
    return r;
}

static FMat4 matRotY(float a)
{
    FMat4 r = matIdentity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[2] = s;
    r.m[8] = -s; r.m[10] = c;
    return r;
}

static FMat4 matRotZ(float a)
{
    FMat4 r = matIdentity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[1] = -s;
    r.m[4] = s; r.m[5] = c;
    return r;
}

static FVec3 transformPoint(const FMat4& m, FVec3 p)
{
    return {
        m.m[0] * p.x + m.m[1] * p.y + m.m[2] * p.z + m.m[3],
        m.m[4] * p.x + m.m[5] * p.y + m.m[6] * p.z + m.m[7],
        m.m[8] * p.x + m.m[9] * p.y + m.m[10] * p.z + m.m[11]
    };
}

static FVec3 transformDirection(const FMat4& m, FVec3 p)
{
    return {
        m.m[0] * p.x + m.m[1] * p.y + m.m[2] * p.z,
        m.m[4] * p.x + m.m[5] * p.y + m.m[6] * p.z,
        m.m[8] * p.x + m.m[9] * p.y + m.m[10] * p.z
    };
}

static FVec3 normalizeDirection(FVec3 v)
{
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq <= 0.0000001f)
        return {0.0f, 0.0f, 0.0f};
    const float invLen = 1.0f / std::sqrt(lenSq);
    return {v.x * invLen, v.y * invLen, v.z * invLen};
}

static float saturnAngle(s32 raw)
{
    const s32 units = raw >> 16;
    constexpr float kTwoPi = 6.28318530717958647692f;
    return static_cast<float>(units & 0xFFF) * (kTwoPi / 4096.0f);
}

struct BonePoseRaw {
    s32 tx, ty, tz;
    s32 rx, ry, rz;
};

static bool readPose(const std::vector<u8>& bundle, u32 poseBase,
                     unsigned bone, BonePoseRaw& out)
{
    const u64 p = static_cast<u64>(poseBase) + static_cast<u64>(bone) * 36u;
    if (p + 36 > bundle.size())
        return false;

    const u8* b = bundle.data() + static_cast<u32>(p);
    out.tx = static_cast<s32>(readBE32Raw(b + 0));
    out.ty = static_cast<s32>(readBE32Raw(b + 4));
    out.tz = static_cast<s32>(readBE32Raw(b + 8));
    out.rx = static_cast<s32>(readBE32Raw(b + 12));
    out.ry = static_cast<s32>(readBE32Raw(b + 16));
    out.rz = static_cast<s32>(readBE32Raw(b + 20));
    return true;
}

static FMat4 poseMatrix(const BonePoseRaw& p)
{
    const float fx = static_cast<float>(p.tx) / 65536.0f;
    const float fy = static_cast<float>(p.ty) / 65536.0f;
    const float fz = static_cast<float>(p.tz) / 65536.0f;

    // Matches Azel's translate + rotateCurrentMatrixZYX traversal.
    FMat4 m = matTranslate(fx, fy, fz);
    m = matMul(m, matRotZ(saturnAngle(p.rz)));
    m = matMul(m, matRotY(saturnAngle(p.ry)));
    m = matMul(m, matRotX(saturnAngle(p.rx)));
    return m;
}

static void polygonColor(unsigned polygon, unsigned model,
                         std::uint8_t& r, std::uint8_t& g, std::uint8_t& b)
{
    // Deterministic high-separation debug palette, not game color data.
    const u32 h = 0x9E3779B9u * (polygon + 1u) ^ (0x85EBCA6Bu * (model + 3u));
    r = static_cast<u8>(80u + ((h >> 0) & 0xAFu));
    g = static_cast<u8>(80u + ((h >> 8) & 0xAFu));
    b = static_cast<u8>(80u + ((h >> 16) & 0xAFu));
}

static bool appendModelDebugGeometry(const std::vector<u8>& bundle, u32 modelOffset,
                                     const FMat4& world, unsigned modelNumber,
                                     lagi::azel::BasicWingDebugMesh& out)
{
    if (modelOffset + 0x14 > bundle.size())
        return false;

    const u8* base = bundle.data();
    const u32 numVertices = readBE32Raw(base + modelOffset + 4);
    const u32 verticesOffset = readBE32Raw(base + modelOffset + 8);
    if (numVertices == 0 || numVertices > 65535)
        return false;
    if (static_cast<u64>(verticesOffset) + static_cast<u64>(numVertices) * 6u > bundle.size())
        return false;

    std::vector<FVec3> verts(numVertices);
    for (u32 i = 0; i < numVertices; ++i) {
        const u8* v = base + verticesOffset + i * 6u;
        const s16 x = static_cast<s16>(readBE16Raw(v + 0));
        const s16 y = static_cast<s16>(readBE16Raw(v + 2));
        const s16 z = static_cast<s16>(readBE16Raw(v + 4));
        verts[i] = transformPoint(world, {
            static_cast<float>(x) / 4096.0f,
            static_cast<float>(y) / 4096.0f,
            static_cast<float>(z) / 4096.0f
        });
    }

    u32 p = modelOffset + 0x0C;
    unsigned localPoly = 0;
    while (true) {
        if (p + 8 > bundle.size())
            return false;

        u16 idx[4] = {
            readBE16Raw(base + p + 0), readBE16Raw(base + p + 2),
            readBE16Raw(base + p + 4), readBE16Raw(base + p + 6)
        };

        if (idx[0] == 0 && idx[1] == 0 && idx[2] == 0 && idx[3] == 0)
            break;
        for (int i = 0; i < 4; ++i)
            if (idx[i] >= numVertices) return false;

        p += 8;
        if (p + 12 > bundle.size()) return false;

        // Preserve the six raw words exactly as Azel's sProcessed3dModel
        // does. The final word is used by Azel's texture path as CMDSIZE.
        lagi::azel::SaturnPolygonRecord record{};
        for (int i = 0; i < 4; ++i)
            record.indices[i] = idx[i];
        record.lightingControl = readBE16Raw(base + p + 0);
        record.cmdCtrl = readBE16Raw(base + p + 2);
        record.cmdPmod = readBE16Raw(base + p + 4);
        record.cmdColr = readBE16Raw(base + p + 6);
        record.cmdSrca = readBE16Raw(base + p + 8);
        record.cmdSize = readBE16Raw(base + p + 10);
        record.model = modelNumber;
        record.polygonInModel = localPoly;

        p += 12;

        const unsigned lightingMode = record.lightingMode();
        switch (lightingMode) {
            case 0:
                break;

            case 1: {
                // Azel readExtraData(..., false): one s16x3 normal,
                // followed by two bytes of padding.
                if (p + 8 > bundle.size()) return false;
                record.lighting[0].normal[0] =
                    static_cast<s16>(readBE16Raw(base + p + 0));
                record.lighting[0].normal[1] =
                    static_cast<s16>(readBE16Raw(base + p + 2));
                record.lighting[0].normal[2] =
                    static_cast<s16>(readBE16Raw(base + p + 4));
                record.lighting[0].hasColor = false;
                record.lightingCount = 1;
                p += 8;
                break;
            }

            case 2: {
                // Four corners, each: s16x3 normal + u16x3 color.
                if (p + 48 > bundle.size()) return false;
                for (unsigned corner = 0; corner < 4; ++corner) {
                    const u32 q = p + corner * 12u;
                    record.lighting[corner].normal[0] =
                        static_cast<s16>(readBE16Raw(base + q + 0));
                    record.lighting[corner].normal[1] =
                        static_cast<s16>(readBE16Raw(base + q + 2));
                    record.lighting[corner].normal[2] =
                        static_cast<s16>(readBE16Raw(base + q + 4));
                    record.lighting[corner].color[0] =
                        readBE16Raw(base + q + 6);
                    record.lighting[corner].color[1] =
                        readBE16Raw(base + q + 8);
                    record.lighting[corner].color[2] =
                        readBE16Raw(base + q + 10);
                    record.lighting[corner].hasColor = true;
                }
                record.lightingCount = 4;
                p += 48;
                break;
            }

            case 3: {
                // Four corners, each: s16x3 normal only.
                if (p + 24 > bundle.size()) return false;
                for (unsigned corner = 0; corner < 4; ++corner) {
                    const u32 q = p + corner * 6u;
                    record.lighting[corner].normal[0] =
                        static_cast<s16>(readBE16Raw(base + q + 0));
                    record.lighting[corner].normal[1] =
                        static_cast<s16>(readBE16Raw(base + q + 2));
                    record.lighting[corner].normal[2] =
                        static_cast<s16>(readBE16Raw(base + q + 4));
                    record.lighting[corner].hasColor = false;
                }
                record.lightingCount = 4;
                p += 24;
                break;
            }
        }

        out.polygonRecords.push_back(record);

        std::uint8_t r, g, b;
        polygonColor(out.polygons, modelNumber, r, g, b);

        // Preserve Saturn quad identity: both generated triangles use one color.
        const int tri[6] = {0, 1, 2, 0, 2, 3};

        // Controlled Gouraud diagnostic. The standalone viewer does not yet
        // carry PDS's live light vector/color/falloff state, so use one fixed
        // normalized direction and only test the preserved mode-3 normals,
        // hierarchy rotation, corner association, and interpolation.
        const FVec3 debugLight =
            normalizeDirection({0.35f, -0.55f, 0.76f});
        std::uint8_t cornerShade[4] = {64, 64, 64, 64};
        lagi::azel::SaturnGouraud555Quad gouraud555{};
        lagi::azel::SaturnLightingNormalQuad lightingNormals{};

        if (record.lightingMode() == 3 && record.lightingCount == 4) {
            for (int corner = 0; corner < 4; ++corner) {
                const FVec3 rawNormal = {
                    static_cast<float>(record.lighting[corner].normal[0]),
                    static_cast<float>(record.lighting[corner].normal[1]),
                    static_cast<float>(record.lighting[corner].normal[2])
                };
                const FVec3 normal =
                    normalizeDirection(transformDirection(world, rawNormal));

                lightingNormals.corner[corner][0] = normal.x;
                lightingNormals.corner[corner][1] = normal.y;
                lightingNormals.corner[corner][2] = normal.z;

                float signedDot =
                    normal.x * debugLight.x +
                    normal.y * debugLight.y +
                    normal.z * debugLight.z;
                signedDot =
                    std::max(-1.0f, std::min(1.0f, signedDot));

                // Keep the existing grayscale diagnostic unchanged.
                const float signed01 = signedDot * 0.5f + 0.5f;
                const float level = 0.35f + signed01 * 0.65f;
                cornerShade[corner] =
                    static_cast<std::uint8_t>(level * 255.0f + 0.5f);

                // Azel's Saturn-accurate path stores Gouraud as a 5-bit
                // additive offset around neutral value 16:
                //     offset = (gouraud5 - 16) / 31
                //
                // We do not yet have the live PDS light/falloff state in this
                // standalone viewer, so retain the fixed diagnostic light and
                // use an 8/31 ambient/falloff floor plus positive directional
                // contribution. The quantization and additive representation
                // match Azel; only the source light/falloff is provisional.
                const float diffuse = std::max(0.0f, signedDot);
                const int ambient5 = 8;
                int gouraud5 =
                    ambient5 +
                    static_cast<int>(
                        diffuse * static_cast<float>(31 - ambient5) + 0.5f);
                gouraud5 = std::max(0, std::min(31, gouraud5));
                const float offset =
                    (static_cast<float>(gouraud5) - 16.0f) / 31.0f;

                gouraud555.corner[corner][0] = offset;
                gouraud555.corner[corner][1] = offset;
                gouraud555.corner[corner][2] = offset;
            }
        }

        out.gouraud555.push_back(gouraud555);
        out.lightingNormals.push_back(lightingNormals);

        for (int k = 0; k < 6; ++k) {
            const int corner = tri[k];
            const FVec3& q = verts[idx[corner]];
            out.vertices.push_back({q.x, q.y, q.z, r, g, b, 255});

            const std::uint8_t shade = cornerShade[corner];
            out.lightingVertices.push_back(
                {q.x, q.y, q.z, shade, shade, shade, 255});
        }

        ++out.polygons;
        ++localPoly;
        if (localPoly > 65535) return false;
    }

    ++out.models;
    return true;
}

static bool buildMeshTraverse(const std::vector<u8>& bundle, u32 nodeOffset,
                              u32 poseBase, unsigned& boneIndex,
                              const FMat4& parent,
                              lagi::azel::BasicWingDebugMesh& out,
                              unsigned depth,
                              const std::vector<BonePoseRaw>* poseOverride = nullptr)
{
    if (!nodeOffset) return true;
    if (depth > 256 || nodeOffset + 12 > bundle.size()) return false;

    do {
        BonePoseRaw pose{};
        if (poseOverride) {
            if (boneIndex >= poseOverride->size())
                return false;
            pose = (*poseOverride)[boneIndex];
        } else if (!readPose(bundle, poseBase, boneIndex, pose)) {
            return false;
        }

        const FMat4 world = matMul(parent, poseMatrix(pose));

        const u32 modelOffset = readBE32Raw(bundle.data() + nodeOffset + 0);
        const u32 childOffset = readBE32Raw(bundle.data() + nodeOffset + 4);
        const u32 nextOffset = readBE32Raw(bundle.data() + nodeOffset + 8);

        if (modelOffset && !appendModelDebugGeometry(bundle, modelOffset, world, out.models, out))
            return false;

        ++boneIndex;

        if (childOffset && !buildMeshTraverse(bundle, childOffset, poseBase,
                                               boneIndex, world, out, depth + 1,
                                               poseOverride))
            return false;

        nodeOffset = nextOffset;
    } while (nodeOffset);

    return true;
}


struct RawAnimationTrack {
    s16 length[9]{};
    std::vector<s16> data[9];
};

struct RawAnimation {
    u16 flags = 0;
    u16 bones = 0;
    u16 frames = 0;
    std::vector<RawAnimationTrack> tracks;
};

struct CompressedTrackState {
    unsigned currentStep = 0;
    int delay = 0;
    s32 value = 0;
};

static bool parseRawAnimation(const std::vector<u8>& bundle,
                              u32 tableOffset,
                              RawAnimation& out)
{
    if (tableOffset + 4 > bundle.size())
        return false;

    const u32 animOffset = readBE32Raw(bundle.data() + tableOffset);
    if (!animOffset || animOffset + 12 > bundle.size())
        return false;

    const u8* base = bundle.data();
    out = {};
    out.flags = readBE16Raw(base + animOffset + 0);
    out.bones = readBE16Raw(base + animOffset + 2);
    out.frames = readBE16Raw(base + animOffset + 4);
    const u32 trackHeaderOffset =
        readBE32Raw(base + animOffset + 8);

    if (!out.flags || !out.bones || !out.frames ||
        out.bones > 128 || out.frames > 1024)
        return false;

    out.tracks.resize(out.bones);

    for (unsigned bone = 0; bone < out.bones; ++bone) {
        const u64 header =
            static_cast<u64>(animOffset) +
            static_cast<u64>(trackHeaderOffset) +
            static_cast<u64>(bone) * 0x38u;
        if (header + 0x38u > bundle.size())
            return false;

        RawAnimationTrack& track = out.tracks[bone];
        for (int channel = 0; channel < 9; ++channel)
            track.length[channel] =
                static_cast<s16>(
                    readBE16Raw(base + header + channel * 2u));

        for (int channel = 0; channel < 9; ++channel) {
            const u32 dataOffset =
                readBE32Raw(
                    base + header + 0x14u + channel * 4u);
            const int length = track.length[channel];
            if (length <= 0)
                continue;

            const u64 dataStart =
                static_cast<u64>(animOffset) + dataOffset;
            const u64 dataBytes =
                static_cast<u64>(length) * 2u;
            if (dataStart + dataBytes > bundle.size())
                return false;

            track.data[channel].resize(
                static_cast<std::size_t>(length));
            for (int i = 0; i < length; ++i) {
                track.data[channel][i] =
                    static_cast<s16>(
                        readBE16Raw(
                            base + dataStart +
                            static_cast<u64>(i) * 2u));
            }
        }
    }

    return true;
}

static s32 stepCompressedTrack(CompressedTrackState& state,
                               const std::vector<s16>& data)
{
    if (data.empty())
        return 0;

    if (state.delay > 0) {
        --state.delay;
        return state.value;
    }

    if (state.currentStep != 0) {
        const u16 packed =
            static_cast<u16>(data[state.currentStep]);
        state.delay =
            static_cast<int>(packed & 0xFu) - 1;
        state.value =
            static_cast<s16>(packed & 0xFFF0u);
    } else {
        state.delay = 0;
        state.value =
            static_cast<s32>(data[0]) * 16;
    }

    ++state.currentStep;
    if (state.currentStep >= data.size())
        state.currentStep = 0;

    return state.value;
}

static bool buildMorphScreenAnimation(
    const std::vector<u8>& bundle,
    u32 hierarchyOffset,
    u32 poseOffset,
    lagi::azel::BasicWingDebugMesh& out)
{
    // Basic Wing dragonAnimOffsets[0] in pinned Azel.
    constexpr u32 kMorphAnimationTableOffset = 0x10Cu;

    RawAnimation anim{};
    if (!parseRawAnimation(
            bundle, kMorphAnimationTableOffset, anim))
        return false;

    if (anim.bones != 31u)
        return false;

    std::vector<BonePoseRaw> staticPose(anim.bones);
    for (unsigned bone = 0; bone < anim.bones; ++bone) {
        if (!readPose(
                bundle, poseOffset, bone, staticPose[bone]))
            return false;
    }

    std::vector<BonePoseRaw> pose = staticPose;
    std::vector<std::array<CompressedTrackState, 3>>
        compressed(anim.bones);
    std::vector<std::array<s32, 3>>
        fractionalStep(anim.bones);

    out.animationFrames.clear();
    out.animationFrames.reserve(anim.frames);
    out.animationFlags = anim.flags;
    out.animationFrameCount = anim.frames;

    const unsigned type = anim.flags & 7u;
    if (type != 0u && type != 1u &&
        type != 3u && type != 4u && type != 5u) {
        lagi::platform::logging::writef(
            "[Dragon] morph animation 0x10C unsupported type %u flags 0x%X\n",
            type, anim.flags);
        return false;
    }

    // The morph-screen flap is fundamentally a skeletal rotation animation.
    // Reproduce Azel's rotation update modes exactly. Static translations are
    // retained, keeping the viewer centered while preserving the wing/body
    // articulation used by the menu.
    for (unsigned frame = 0; frame < anim.frames; ++frame) {
        if (anim.flags & 0x10u) {
            for (unsigned bone = 0; bone < anim.bones; ++bone) {
                const RawAnimationTrack& track =
                    anim.tracks[bone];

                for (int axis = 0; axis < 3; ++axis) {
                    const std::vector<s16>& data =
                        track.data[3 + axis];

                    if (data.empty())
                        continue;

                    s32& rotation =
                        axis == 0 ? pose[bone].rx :
                        axis == 1 ? pose[bone].ry :
                                    pose[bone].rz;

                    if (type == 0u) {
                        if (frame >= data.size())
                            return false;
                        rotation =
                            static_cast<s32>(data[frame]) *
                            0x10000;
                    } else if (type == 3u) {
                        if ((frame & 3u) == 0u) {
                            const unsigned key = frame / 4u;
                            if (key >= data.size())
                                return false;
                            rotation =
                                static_cast<s32>(data[key]) *
                                0x10000;

                            if (frame + 1u < anim.frames &&
                                key + 1u < data.size()) {
                                fractionalStep[bone][axis] =
                                    (static_cast<s32>(
                                        data[key + 1u] -
                                        data[key]) *
                                     0x10000) / 4;
                            } else {
                                fractionalStep[bone][axis] = 0;
                            }
                        } else {
                            rotation +=
                                fractionalStep[bone][axis];
                        }
                    } else if (type == 1u) {
                        const s32 delta =
                            stepCompressedTrack(
                                compressed[bone][axis], data) *
                            0x1000;
                        if (frame == 0u)
                            rotation = delta;
                        else
                            rotation += delta;
                    } else {
                        const unsigned divisor =
                            type == 4u ? 2u : 4u;
                        if ((frame % divisor) == 0u) {
                            if (frame == 0u) {
                                rotation =
                                    stepCompressedTrack(
                                        compressed[bone][axis],
                                        data) *
                                    0x1000;
                            } else {
                                rotation +=
                                    fractionalStep[bone][axis];
                            }

                            if (frame + 1u < anim.frames) {
                                fractionalStep[bone][axis] =
                                    (stepCompressedTrack(
                                        compressed[bone][axis],
                                        data) *
                                     0x1000) /
                                    static_cast<s32>(divisor);
                            } else {
                                fractionalStep[bone][axis] = 0;
                            }
                        } else {
                            rotation +=
                                fractionalStep[bone][axis];
                        }
                    }
                }
            }
        }

        lagi::azel::BasicWingDebugMesh frameMesh{};
        unsigned boneIndex = 0;
        if (!buildMeshTraverse(
                bundle, hierarchyOffset, poseOffset,
                boneIndex, matIdentity(), frameMesh, 0,
                &pose))
            return false;

        if (boneIndex != anim.bones ||
            frameMesh.vertices.size() != out.vertices.size() ||
            frameMesh.lightingNormals.size() !=
                out.lightingNormals.size())
            return false;

        lagi::azel::BasicWingAnimationFrame outFrame{};
        outFrame.vertices =
            std::move(frameMesh.vertices);
        outFrame.lightingNormals =
            std::move(frameMesh.lightingNormals);
        out.animationFrames.push_back(
            std::move(outFrame));
    }

    out.animationValid =
        out.animationFrames.size() == anim.frames;

    lagi::platform::logging::writef(
        "[Dragon] morph animation 0x10C: flags 0x%X type %u, %u bones, %u frames, %s\n",
        anim.flags, type, anim.bones, anim.frames,
        out.animationValid ? "decoded" : "INVALID");

    return out.animationValid;
}


static std::uint32_t rgb555ToRgba8888(u16 color)
{
    // Saturn RGB555: bits 0-4 R, 5-9 G, 10-14 B, bit 15 direct-color flag.
    const std::uint32_t r5 = color & 0x1Fu;
    const std::uint32_t g5 = (color >> 5) & 0x1Fu;
    const std::uint32_t b5 = (color >> 10) & 0x1Fu;
    const std::uint32_t r8 = (r5 << 3) | (r5 >> 2);
    const std::uint32_t g8 = (g5 << 3) | (g5 >> 2);
    const std::uint32_t b8 = (b5 << 3) | (b5 >> 2);
    return 0xFF000000u | r8 | (g8 << 8) | (b8 << 16);
}

static bool sameTextureDescriptor(const lagi::azel::SaturnPolygonRecord& a,
                                  const lagi::azel::SaturnPolygonRecord& b)
{
    return a.cmdPmod == b.cmdPmod &&
           a.cmdColr == b.cmdColr &&
           a.cmdSrca == b.cmdSrca &&
           a.cmdSize == b.cmdSize;
}

static void validateMode1Textures(
    const std::vector<u8>& cgb,
    lagi::azel::BasicWingDebugMesh& out)
{
    out.uniqueTextures = 0;
    out.decodedTextures = 0;
    out.decodedPixels = 0;
    out.transparentPixels = 0;
    out.endCodePixels = 0;
    out.directRgb555Pixels = 0;
    out.indirectCramPixels = 0;
    out.mode1DecodeValid = false;
    out.mode1DecodeFullyResolved = false;
    out.decodedTextureData.clear();
    out.polygonTextureIndices.assign(
        out.polygonRecords.size(), static_cast<std::uint16_t>(0xFFFFu));

    bool valid = true;

    for (std::size_t i = 0; i < out.polygonRecords.size(); ++i) {
        const auto& record = out.polygonRecords[i];

        int existingTexture = -1;
        for (std::size_t t = 0; t < out.decodedTextureData.size(); ++t) {
            const auto& texture = out.decodedTextureData[t];
            if (record.cmdPmod == texture.cmdPmod &&
                record.cmdColr == texture.cmdColr &&
                record.cmdSrca == texture.cmdSrca &&
                record.cmdSize == texture.cmdSize) {
                existingTexture = static_cast<int>(t);
                break;
            }
        }

        if (existingTexture >= 0) {
            out.polygonTextureIndices[i] =
                static_cast<std::uint16_t>(existingTexture);
            continue;
        }

        ++out.uniqueTextures;

        const unsigned width = record.textureWidth();
        const unsigned height = record.textureHeight();
        const unsigned texAddress = record.textureByteAddress();
        const unsigned lutAddress =
            static_cast<unsigned>(record.cmdColr) << 3;

        if (record.colorMode() != 1 || width == 0 || height == 0) {
            valid = false;
            continue;
        }

        const unsigned texBytes = (width * height) / 2u;
        if (texAddress > cgb.size() ||
            texBytes > cgb.size() ||
            static_cast<std::size_t>(texAddress) + texBytes > cgb.size() ||
            static_cast<std::size_t>(lutAddress) + 32u > cgb.size()) {
            valid = false;
            continue;
        }

        lagi::azel::DecodedMode1Texture texture{};
        texture.cmdPmod = record.cmdPmod;
        texture.cmdColr = record.cmdColr;
        texture.cmdSrca = record.cmdSrca;
        texture.cmdSize = record.cmdSize;
        texture.width = width;
        texture.height = height;
        texture.rgba.assign(width * height, 0u);

        const bool spd = (record.cmdPmod & 0x40u) != 0;
        const bool endDisabled = (record.cmdPmod & 0x80u) != 0;
        const bool endMode = (record.cmdPmod & 0x20u) == 0;

        unsigned pixel = 0;
        for (unsigned y = 0; y < height; ++y) {
            unsigned endCount = 0;

            for (unsigned x = 0; x < width; ++x, ++pixel) {
                const unsigned byteOffset =
                    texAddress + (x + y * width) / 2u;
                const u8 packed = cgb[byteOffset];
                const u8 dot =
                    (x & 1u) ? (packed & 0x0Fu) : (packed >> 4);

                if (endMode && endCount >= 2u) {
                    ++out.transparentPixels;
                    continue;
                }

                if (dot == 0 && !spd) {
                    ++out.transparentPixels;
                    continue;
                }

                if (dot == 0x0F && !endDisabled) {
                    ++endCount;
                    ++out.endCodePixels;
                    ++out.transparentPixels;
                    continue;
                }

                const u16 lutColor =
                    readBE16Raw(cgb.data() + lutAddress + dot * 2u);

                if (lutColor & 0x8000u) {
                    texture.rgba[pixel] = rgb555ToRgba8888(lutColor);
                    ++out.directRgb555Pixels;
                } else if (lutColor != 0) {
                    ++out.indirectCramPixels;
                } else {
                    ++out.transparentPixels;
                }

                ++out.decodedPixels;
            }
        }

        const std::uint16_t textureIndex =
            static_cast<std::uint16_t>(out.decodedTextureData.size());
        out.decodedTextureData.push_back(std::move(texture));
        out.polygonTextureIndices[i] = textureIndex;
        ++out.decodedTextures;
    }

    out.mode1DecodeValid =
        valid &&
        out.uniqueTextures != 0 &&
        out.decodedTextures == out.uniqueTextures &&
        out.polygonTextureIndices.size() == out.polygonRecords.size();

    if (out.mode1DecodeValid) {
        for (const std::uint16_t index : out.polygonTextureIndices) {
            if (index == 0xFFFFu ||
                index >= out.decodedTextureData.size()) {
                out.mode1DecodeValid = false;
                break;
            }
        }
    }

    out.mode1DecodeFullyResolved =
        out.mode1DecodeValid &&
        out.indirectCramPixels == 0;

    lagi::platform::logging::writef(
        "[Dragon] MODE1 decode: %u/%u textures, %u pixels, %u transparent, %u end-code\n",
        out.decodedTextures, out.uniqueTextures, out.decodedPixels,
        out.transparentPixels, out.endCodePixels);
    lagi::platform::logging::writef(
        "[Dragon] MODE1 LUT: %u direct RGB555 pixels, %u indirect CRAM pixels, %s\n",
        out.directRgb555Pixels, out.indirectCramPixels,
        out.mode1DecodeFullyResolved
            ? "fully resolved from CGB"
            : (out.mode1DecodeValid ? "CRAM resolution required" : "DECODE INVALID"));
}

} // anonymous namespace

namespace lagi::azel {

bool build_basic_wing_debug_mesh(BasicWingDebugMesh& out)
{
    out = {};

    std::vector<u8> mcb;
    if (!lagi::disc::read_file("DRAGON0.MCB", mcb) || mcb.size() < 16)
        return false;

    const sDragonMorphModels& base =
        gDragonMorphDataPerLevel[DR_LEVEL_0_BASIC_WING].m_m8[0];

    if (static_cast<u32>(base.m0_modelIndex) + 4u > mcb.size() ||
        static_cast<u32>(base.m4_poseModelIndex) + 4u > mcb.size())
        return false;

    const u32 hierarchyOffset = readBE32Raw(mcb.data() + base.m0_modelIndex);
    const u32 poseOffset = readBE32Raw(mcb.data() + base.m4_poseModelIndex);
    if (!hierarchyOffset || !poseOffset)
        return false;

    unsigned boneIndex = 0;
    if (!buildMeshTraverse(mcb, hierarchyOffset, poseOffset, boneIndex,
                           matIdentity(), out, 0))
        return false;

    unsigned uniqueTextureDescriptors = 0;
    unsigned colorModeCounts[8]{};
    unsigned flippedPolygons = 0;
    unsigned minTextureAddress = 0xFFFFFFFFu;
    unsigned maxTextureEnd = 0;
    unsigned maxLutEnd = 0;
    unsigned lightingModeCounts[4]{};
    unsigned lightingExtraRecords = 0;
    unsigned lightingColoredRecords = 0;
    bool lightingPayloadValid = true;

    for (std::size_t i = 0; i < out.polygonRecords.size(); ++i) {
        const SaturnPolygonRecord& record = out.polygonRecords[i];
        ++colorModeCounts[record.colorMode()];
        if (record.textureFlip() != 0)
            ++flippedPolygons;

        const unsigned lightingMode = record.lightingMode();
        ++lightingModeCounts[lightingMode];
        lightingExtraRecords += record.lightingCount;

        const unsigned expectedLightingCount =
            lightingMode == 0 ? 0u :
            lightingMode == 1 ? 1u : 4u;
        if (record.lightingCount != expectedLightingCount)
            lightingPayloadValid = false;

        for (unsigned n = 0; n < record.lightingCount; ++n) {
            if (record.lighting[n].hasColor)
                ++lightingColoredRecords;
            if ((lightingMode == 2) != record.lighting[n].hasColor)
                lightingPayloadValid = false;
        }

        const unsigned address = record.textureByteAddress();
        const unsigned width = record.textureWidth();
        const unsigned height = record.textureHeight();
        const unsigned colorMode = record.colorMode();
        const unsigned bytesPerTexture =
            colorMode <= 1 ? (width * height) / 2 :
            colorMode == 5 ? width * height * 2 :
                             width * height;
        minTextureAddress = std::min(minTextureAddress, address);
        maxTextureEnd = std::max(maxTextureEnd, address + bytesPerTexture);

        // Basic Wing uses color mode 1 in current hardware data. In Azel's
        // decoder that mode uses a 16-entry LUT stored in VDP1 memory at
        // CMDCOLR * 8, i.e. 32 bytes from this relative CGB address.
        if (colorMode == 1) {
            const unsigned lutAddress =
                static_cast<unsigned>(record.cmdColr) << 3;
            maxLutEnd = std::max(maxLutEnd, lutAddress + 32u);
        }

        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            const SaturnPolygonRecord& other = out.polygonRecords[j];
            if (record.cmdPmod == other.cmdPmod &&
                record.cmdColr == other.cmdColr &&
                record.cmdSrca == other.cmdSrca &&
                record.cmdSize == other.cmdSize) {
                seen = true;
                break;
            }
        }
        if (!seen)
            ++uniqueTextureDescriptors;
    }

    lagi::platform::logging::writef("[Dragon] debug mesh built: %u bones, %u models, %u polys, %u triangle vertices\n",
                boneIndex, out.models, out.polygons,
                static_cast<unsigned>(out.vertices.size()));
    lagi::platform::logging::writef("[Dragon] VDP1 polygon records: %u records, %u unique texture descriptors, %u flipped\n",
                static_cast<unsigned>(out.polygonRecords.size()),
                uniqueTextureDescriptors, flippedPolygons);
    lagi::platform::logging::writef("[Dragon] VDP1 texture address span: 0x%X-0x%X; color modes %u/%u/%u/%u/%u/%u/%u/%u\n",
                minTextureAddress == 0xFFFFFFFFu ? 0u : minTextureAddress,
                maxTextureEnd,
                colorModeCounts[0], colorModeCounts[1], colorModeCounts[2],
                colorModeCounts[3], colorModeCounts[4], colorModeCounts[5],
                colorModeCounts[6], colorModeCounts[7]);

    for (unsigned i = 0; i < 4; ++i)
        out.lightingModeCounts[i] = lightingModeCounts[i];
    out.lightingExtraRecords = lightingExtraRecords;
    out.lightingColoredRecords = lightingColoredRecords;
    out.lightingPayloadValid = lightingPayloadValid;

    lagi::platform::logging::writef(
        "[Dragon] lighting modes 0/1/2/3: %u/%u/%u/%u; extras %u; colored %u; %s\n",
        out.lightingModeCounts[0], out.lightingModeCounts[1],
        out.lightingModeCounts[2], out.lightingModeCounts[3],
        out.lightingExtraRecords, out.lightingColoredRecords,
        out.lightingPayloadValid ? "payload valid" : "PAYLOAD INVALID");

    // Trace the original load path:
    //   DRAGON0.CGB -> VDP1 byte offset 0x12000
    //   DRAGON0.MCB relocation = 0x2400 address units
    //   0x2400 << 3 == 0x12000
    //
    // Our preserved polygon command words are the pre-relocation values, so
    // CMDSRCA<<3 and CMDCOLR<<3 are directly relative to the start of CGB.
    std::vector<u8> cgb;
    if (lagi::disc::read_file("DRAGON0.CGB", cgb)) {
        out.cgbBytes = static_cast<unsigned>(cgb.size());
        out.maxTextureEnd = maxTextureEnd;
        out.maxLutEnd = maxLutEnd;
        out.cgbReferencesValid =
            maxTextureEnd <= out.cgbBytes &&
            maxLutEnd <= out.cgbBytes;

        lagi::platform::logging::writef(
            "[Dragon] DRAGON0.CGB: %u bytes; original VDP1 base 0x12000 / reloc 0x2400\n",
            out.cgbBytes);
        lagi::platform::logging::writef(
            "[Dragon] CGB refs: texture end 0x%X, LUT end 0x%X, %s\n",
            out.maxTextureEnd, out.maxLutEnd,
            out.cgbReferencesValid ? "all in range" : "OUT OF RANGE");

        if (out.cgbReferencesValid)
            validateMode1Textures(cgb, out);
    } else {
        out.cgbBytes = 0;
        out.maxTextureEnd = maxTextureEnd;
        out.maxLutEnd = maxLutEnd;
        out.cgbReferencesValid = false;
        lagi::platform::logging::writef(
            "[Dragon] DRAGON0.CGB could not be read for reference validation\n");
    }

    return boneIndex == 31 && out.models == 31 &&
           out.polygons == 212 &&
           out.polygonRecords.size() == out.polygons &&
           out.gouraud555.size() == out.polygons &&
           out.lightingNormals.size() == out.polygons &&
           out.vertices.size() == 212u * 6u &&
           out.lightingVertices.size() == out.vertices.size() &&
           out.lightingPayloadValid;
}

} // namespace lagi::azel
