#include "lagi/debug_mesh.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <utility>

s8 readSaturnS8(sSaturnPtr ptr);
s16 readSaturnS16(sSaturnPtr ptr);
s32 readSaturnS32(sSaturnPtr ptr);
u16 readSaturnU16(sSaturnPtr ptr);
u32 readSaturnU32(sSaturnPtr ptr);
fixedPoint readSaturnFP(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);

namespace lagi::azel {

namespace {

struct RawModelQuadExtra {
    std::int16_t normal[3]{};
    std::uint16_t color[3]{};
    bool hasColor = false;
};

struct RawModelQuad {
    std::uint16_t indices[4]{};
    std::uint16_t lightingControl = 0;
    std::uint16_t cmdCtrl = 0;
    std::uint16_t cmdPmod = 0;
    std::uint16_t cmdColr = 0;
    std::uint16_t cmdSrca = 0;
    std::uint16_t cmdSize = 0;
    std::vector<RawModelQuadExtra> extra;
};

struct RawModel {
    std::vector<std::array<std::int16_t,3>> vertices;
    std::vector<RawModelQuad> quads;
};

struct Mat3 {
    float m[3][3]{};
};

static Mat3 identity3()
{
    Mat3 r{};
    r.m[0][0] = r.m[1][1] = r.m[2][2] = 1.0f;
    return r;
}

static Mat3 mul3(const Mat3& a, const Mat3& b)
{
    Mat3 r{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

static float angle12(std::int16_t raw)
{
    constexpr float kTau = 6.28318530717958647692f;
    const unsigned int a = static_cast<unsigned int>(
        static_cast<std::uint16_t>(raw)) & 0x0FFFu;
    return static_cast<float>(a) * (kTau / 4096.0f);
}

static Mat3 rotX(std::int16_t raw)
{
    Mat3 r = identity3();
    const float a = angle12(raw);
    const float c = std::cos(a);
    const float s = std::sin(a);
    r.m[1][1] = c;  r.m[1][2] = -s;
    r.m[2][1] = s;  r.m[2][2] = c;
    return r;
}

static Mat3 rotY(std::int16_t raw)
{
    Mat3 r = identity3();
    const float a = angle12(raw);
    const float c = std::cos(a);
    const float s = std::sin(a);
    r.m[0][0] = c;  r.m[0][2] = s;
    r.m[2][0] = -s; r.m[2][2] = c;
    return r;
}

static Mat3 rotZ(std::int16_t raw)
{
    Mat3 r = identity3();
    const float a = angle12(raw);
    const float c = std::cos(a);
    const float s = std::sin(a);
    r.m[0][0] = c;  r.m[0][1] = -s;
    r.m[1][0] = s;  r.m[1][1] = c;
    return r;
}

static std::array<float,3> apply(
    const Mat3& m,
    const std::array<float,3>& v)
{
    return {{
        m.m[0][0]*v[0] + m.m[0][1]*v[1] + m.m[0][2]*v[2],
        m.m[1][0]*v[0] + m.m[1][1]*v[1] + m.m[1][2]*v[2],
        m.m[2][0]*v[0] + m.m[2][1]*v[1] + m.m[2][2]*v[2]
    }};
}

static bool beBounds(const std::vector<u8>& data, u32 offset, u32 bytes)
{
    return offset <= data.size() &&
           bytes <= data.size() - offset;
}

static u16 be16(const std::vector<u8>& data, u32 offset)
{
    return static_cast<u16>(
        (static_cast<u16>(data[offset]) << 8) |
        static_cast<u16>(data[offset + 1]));
}

static s16 bes16(const std::vector<u8>& data, u32 offset)
{
    return static_cast<s16>(be16(data, offset));
}

static u32 be32(const std::vector<u8>& data, u32 offset)
{
    return (static_cast<u32>(data[offset]) << 24) |
           (static_cast<u32>(data[offset+1]) << 16) |
           (static_cast<u32>(data[offset+2]) << 8) |
           static_cast<u32>(data[offset+3]);
}

static bool parseRawModel(
    const std::vector<u8>& bundle,
    u32 tableOffset,
    RawModel& out)
{
    out = {};
    if (!beBounds(bundle, tableOffset, 4))
        return false;

    const u32 modelOffset = be32(bundle, tableOffset);
    if (!modelOffset || !beBounds(bundle, modelOffset, 12))
        return false;

    const u32 numVertices = be32(bundle, modelOffset + 4);
    const u32 verticesOffset = be32(bundle, modelOffset + 8);
    if (!numVertices || numVertices > 65535u ||
        !beBounds(bundle, verticesOffset, numVertices * 6u))
        return false;

    out.vertices.reserve(numVertices);
    for (u32 i = 0; i < numVertices; ++i) {
        const u32 p = verticesOffset + i * 6u;
        out.vertices.push_back({{
            bes16(bundle, p + 0),
            bes16(bundle, p + 2),
            bes16(bundle, p + 4)
        }});
    }

    u32 q = modelOffset + 0x0Cu;
    while (true) {
        if (!beBounds(bundle, q, 8))
            return false;

        RawModelQuad quad{};
        for (int i = 0; i < 4; ++i)
            quad.indices[i] = be16(bundle, q + i*2u);

        if (quad.indices[0] == 0 &&
            quad.indices[1] == 0 &&
            quad.indices[2] == 0 &&
            quad.indices[3] == 0)
            break;

        q += 8;
        if (!beBounds(bundle, q, 12))
            return false;

        quad.lightingControl = be16(bundle, q + 0); q += 2;
        quad.cmdCtrl         = be16(bundle, q + 0); q += 2;
        quad.cmdPmod         = be16(bundle, q + 0); q += 2;
        quad.cmdColr         = be16(bundle, q + 0); q += 2;
        quad.cmdSrca         = be16(bundle, q + 0); q += 2;
        quad.cmdSize         = be16(bundle, q + 0); q += 2;

        const unsigned int mode =
            (quad.lightingControl >> 8) & 3u;
        const unsigned int extraCount =
            mode == 1 ? 1u : ((mode == 2 || mode == 3) ? 4u : 0u);

        for (unsigned int i = 0; i < extraCount; ++i) {
            const u32 bytes = mode == 2 ? 12u : 6u;
            if (!beBounds(bundle, q, bytes))
                return false;

            RawModelQuadExtra e{};
            e.normal[0] = bes16(bundle, q + 0);
            e.normal[1] = bes16(bundle, q + 2);
            e.normal[2] = bes16(bundle, q + 4);
            q += 6;

            if (mode == 2) {
                e.color[0] = be16(bundle, q + 0);
                e.color[1] = be16(bundle, q + 2);
                e.color[2] = be16(bundle, q + 4);
                e.hasColor = true;
                q += 6;
            }

            quad.extra.push_back(e);
        }

        if (mode == 1) {
            if (!beBounds(bundle, q, 2))
                return false;
            q += 2;
        }

        for (int i = 0; i < 4; ++i)
            if (quad.indices[i] >= out.vertices.size())
                return false;

        out.quads.push_back(std::move(quad));
        if (out.quads.size() > 20000u)
            return false;
    }

    return !out.quads.empty();
}

} // namespace

bool build_first_ruin_room_debug_mesh(StaticRoomDebugMesh& out)
{
    out = {};

    const TownBootstrapInfo& info = town_bootstrap_info();
    sSaturnMemoryFile* overlay = town_overlay_file();
    if (!info.valid || !overlay)
        return false;

    const sSaturnPtr gridRoot =
        overlay->getSaturnPtr(info.gridEA);
    const sSaturnPtr cell = readSaturnEA(gridRoot);
    if (cell.isNull())
        return false;

    const sSaturnPtr staticList = readSaturnEA(cell + 0x0C);
    if (staticList.isNull())
        return false;

    const char* modelBundleName = nullptr;
    switch (info.setupNpcFileIndex) {
    case 0: modelBundleName = "COMMON3.MCB"; break;
    case 2: modelBundleName = "RUINMP.MCB"; break;
    default:
        lagi::platform::logging::writef(
            "[RoomDebug] unsupported town model bundle index %d\n",
            static_cast<int>(info.setupNpcFileIndex));
        return false;
    }

    std::vector<u8> bundle;
    if (!lagi::disc::read_file(modelBundleName, bundle) ||
        bundle.empty())
        return false;

    sSaturnPtr entry = staticList;
    constexpr unsigned int kMaxObjects = 512;
    constexpr unsigned int kMaxVertices = 65520;
    static constexpr unsigned int triCorners[6] = {0,1,2,0,2,3};

    for (unsigned int objectIndex = 0;
         objectIndex < kMaxObjects; ++objectIndex, entry += 0x18) {
        const s32 lodTableEA = readSaturnS32(entry);
        if (lodTableEA == 0)
            break;

        sSaturnPtr lodTable = readSaturnEA(entry);
        const u16 modelTableOffset = readSaturnU16(lodTable);
        if (!modelTableOffset)
            continue;

        RawModel model;
        if (!parseRawModel(bundle, modelTableOffset, model)) {
            lagi::platform::logging::writef(
                "[RoomDebug] model decode failed: object=%u table=0x%04X\n",
                objectIndex,
                static_cast<unsigned>(modelTableOffset));
            continue;
        }

        const std::array<float,3> translation = {{
            static_cast<float>(readSaturnS32(entry + 4)) / 65536.0f,
            static_cast<float>(readSaturnS32(entry + 8)) / 65536.0f,
            static_cast<float>(readSaturnS32(entry + 12)) / 65536.0f
        }};

        const std::int16_t rx = readSaturnS16(entry + 0x10);
        const std::int16_t ry = readSaturnS16(entry + 0x12);
        const std::int16_t rz = readSaturnS16(entry + 0x14);

        // Mirrors rotateMatrixZYX_s16(): postmultiply Z, then Y, then X.
        const Mat3 rotation =
            mul3(mul3(rotZ(rz), rotY(ry)), rotX(rx));

        for (unsigned int p = 0; p < model.quads.size(); ++p) {
            if (out.vertices.size() + 6u > kMaxVertices) {
                out.truncated = true;
                break;
            }

            const RawModelQuad& q = model.quads[p];
            SaturnPolygonRecord record{};
            for (int i = 0; i < 4; ++i)
                record.indices[i] = q.indices[i];
            record.lightingControl = q.lightingControl;
            record.cmdCtrl = q.cmdCtrl;
            record.cmdPmod = q.cmdPmod;
            record.cmdColr = q.cmdColr;
            record.cmdSrca = q.cmdSrca;
            record.cmdSize = q.cmdSize;
            record.model = objectIndex;
            record.polygonInModel = p;

            record.lightingCount =
                static_cast<std::uint8_t>(
                    std::min<std::size_t>(4u, q.extra.size()));
            for (unsigned int i = 0;
                 i < record.lightingCount; ++i) {
                record.lighting[i].normal[0] = q.extra[i].normal[0];
                record.lighting[i].normal[1] = q.extra[i].normal[1];
                record.lighting[i].normal[2] = q.extra[i].normal[2];
                record.lighting[i].color[0] = q.extra[i].color[0];
                record.lighting[i].color[1] = q.extra[i].color[1];
                record.lighting[i].color[2] = q.extra[i].color[2];
                record.lighting[i].hasColor = q.extra[i].hasColor;
            }

            out.polygonRecords.push_back(record);
            out.gouraud555.push_back({});

            const std::uint8_t r =
                static_cast<std::uint8_t>(64u + (objectIndex * 47u) % 176u);
            const std::uint8_t g =
                static_cast<std::uint8_t>(64u + (objectIndex * 83u) % 176u);
            const std::uint8_t b =
                static_cast<std::uint8_t>(64u + (objectIndex * 29u) % 176u);

            for (unsigned int k = 0; k < 6; ++k) {
                const auto& raw = model.vertices[q.indices[triCorners[k]]];
                const std::array<float,3> local = {{
                    static_cast<float>(raw[0]) / 4096.0f,
                    static_cast<float>(raw[1]) / 4096.0f,
                    static_cast<float>(raw[2]) / 4096.0f
                }};

                auto v = apply(rotation, local);
                v[0] += translation[0];
                v[1] += translation[1];
                v[2] += translation[2];

                DebugColorVertex dv{};
                dv.x = v[0]; dv.y = v[1]; dv.z = v[2];
                dv.r = r; dv.g = g; dv.b = b; dv.a = 255;
                out.vertices.push_back(dv);
                out.lightingVertices.push_back(dv);
            }
        }

        ++out.objects;
        ++out.models;
        if (out.truncated)
            break;
    }

    out.polygons =
        static_cast<unsigned int>(out.polygonRecords.size());

    if (out.vertices.empty() ||
        out.vertices.size() != out.polygons * 6u)
        return false;

    float minV[3] = {
        out.vertices[0].x, out.vertices[0].y, out.vertices[0].z
    };
    float maxV[3] = {minV[0], minV[1], minV[2]};

    for (const auto& v : out.vertices) {
        minV[0] = std::min(minV[0], v.x);
        minV[1] = std::min(minV[1], v.y);
        minV[2] = std::min(minV[2], v.z);
        maxV[0] = std::max(maxV[0], v.x);
        maxV[1] = std::max(maxV[1], v.y);
        maxV[2] = std::max(maxV[2], v.z);
    }

    const float center[3] = {
        (minV[0] + maxV[0]) * 0.5f,
        (minV[1] + maxV[1]) * 0.5f,
        (minV[2] + maxV[2]) * 0.5f
    };
    const float extent = std::max({
        maxV[0]-minV[0],
        maxV[1]-minV[1],
        maxV[2]-minV[2],
        0.001f
    });
    const float scale = 3.0f / extent;

    for (auto& v : out.vertices) {
        v.x = (v.x - center[0]) * scale;
        v.y = (v.y - center[1]) * scale;
        v.z = (v.z - center[2]) * scale;
    }
    out.lightingVertices = out.vertices;

    lagi::platform::logging::writef(
        "[RoomDebug] %s objects=%u models=%u polys=%u verts=%u bundle=%s%s\n",
        info.overlayFile.c_str(),
        out.objects,
        out.models,
        out.polygons,
        static_cast<unsigned>(out.vertices.size()),
        modelBundleName,
        out.truncated ? " TRUNCATED" : "");

    return true;
}

} // namespace lagi::azel
