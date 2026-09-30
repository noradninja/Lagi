#include "lagi/debug_mesh.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using namespace lagi::azel;

static std::uint16_t be16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(p[0]) << 8) | p[1]);
}

static std::uint32_t be32(const std::uint8_t* p)
{
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

static std::int32_t bes32(const std::uint8_t* p)
{
    return static_cast<std::int32_t>(be32(p));
}

struct V3 { float x, y, z; };
struct M4 { float m[16]; };

static M4 identity()
{
    M4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static M4 mul(const M4& a, const M4& b)
{
    M4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                r.m[row * 4 + col] +=
                    a.m[row * 4 + k] * b.m[k * 4 + col];
    return r;
}

static M4 translate(float x, float y, float z)
{
    M4 r = identity();
    r.m[3] = x; r.m[7] = y; r.m[11] = z;
    return r;
}

static M4 rotX(float a)
{
    M4 r = identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[5] = c; r.m[6] = -s;
    r.m[9] = s; r.m[10] = c;
    return r;
}

static M4 rotY(float a)
{
    M4 r = identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[2] = s;
    r.m[8] = -s; r.m[10] = c;
    return r;
}

static M4 rotZ(float a)
{
    M4 r = identity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[1] = -s;
    r.m[4] = s; r.m[5] = c;
    return r;
}

static V3 point(const M4& m, V3 p)
{
    return {
        m.m[0] * p.x + m.m[1] * p.y + m.m[2] * p.z + m.m[3],
        m.m[4] * p.x + m.m[5] * p.y + m.m[6] * p.z + m.m[7],
        m.m[8] * p.x + m.m[9] * p.y + m.m[10] * p.z + m.m[11]
    };
}

static float angle(std::int32_t raw)
{
    constexpr float tau = 6.28318530717958647692f;
    const std::int32_t units = raw >> 16;
    return static_cast<float>(units & 0xFFF) * (tau / 4096.0f);
}

struct Pose {
    std::int32_t tx, ty, tz;
    std::int32_t rx, ry, rz;
};

static bool readPose(const std::vector<std::uint8_t>& mcb,
                     std::uint32_t base, unsigned bone, Pose& out)
{
    const std::uint64_t p =
        static_cast<std::uint64_t>(base) +
        static_cast<std::uint64_t>(bone) * 36u;
    if (p + 36u > mcb.size())
        return false;
    const auto* b = mcb.data() + static_cast<std::uint32_t>(p);
    out.tx = bes32(b + 0); out.ty = bes32(b + 4); out.tz = bes32(b + 8);
    out.rx = bes32(b + 12); out.ry = bes32(b + 16); out.rz = bes32(b + 20);
    return true;
}

static M4 poseMatrix(const Pose& p)
{
    M4 r = translate(
        static_cast<float>(p.tx) / 65536.0f,
        static_cast<float>(p.ty) / 65536.0f,
        static_cast<float>(p.tz) / 65536.0f);
    r = mul(r, rotZ(angle(p.rz)));
    r = mul(r, rotY(angle(p.ry)));
    r = mul(r, rotX(angle(p.rx)));
    return r;
}

static void debugColor(unsigned poly, unsigned model,
                       std::uint8_t& r, std::uint8_t& g, std::uint8_t& b)
{
    const std::uint32_t h =
        0x9E3779B9u * (poly + 1u) ^
        0x85EBCA6Bu * (model + 3u);
    r = static_cast<std::uint8_t>(80u + ((h >> 0) & 0xAFu));
    g = static_cast<std::uint8_t>(80u + ((h >> 8) & 0xAFu));
    b = static_cast<std::uint8_t>(80u + ((h >> 16) & 0xAFu));
}

static bool appendModel(const std::vector<std::uint8_t>& mcb,
                        std::uint32_t modelOffset,
                        const M4& world,
                        unsigned modelNumber,
                        BasicWingDebugMesh& out)
{
    if (modelOffset + 0x14u > mcb.size())
        return false;

    const auto* base = mcb.data();
    const std::uint32_t nv = be32(base + modelOffset + 4);
    const std::uint32_t vo = be32(base + modelOffset + 8);
    if (!nv || nv > 65535u ||
        static_cast<std::uint64_t>(vo) +
            static_cast<std::uint64_t>(nv) * 6u > mcb.size())
        return false;

    std::vector<V3> verts(nv);
    for (std::uint32_t i = 0; i < nv; ++i) {
        const auto* v = base + vo + i * 6u;
        const auto x = static_cast<std::int16_t>(be16(v + 0));
        const auto y = static_cast<std::int16_t>(be16(v + 2));
        const auto z = static_cast<std::int16_t>(be16(v + 4));
        verts[i] = point(world, {
            static_cast<float>(x) / 4096.0f,
            static_cast<float>(y) / 4096.0f,
            static_cast<float>(z) / 4096.0f
        });
    }

    std::uint32_t p = modelOffset + 0x0Cu;
    unsigned localPoly = 0;
    while (true) {
        if (p + 8u > mcb.size())
            return false;
        std::uint16_t idx[4] = {
            be16(base + p + 0), be16(base + p + 2),
            be16(base + p + 4), be16(base + p + 6)
        };
        if (!idx[0] && !idx[1] && !idx[2] && !idx[3])
            break;
        for (auto i : idx)
            if (i >= nv) return false;
        p += 8u;
        if (p + 12u > mcb.size())
            return false;

        SaturnPolygonRecord rec{};
        for (int i = 0; i < 4; ++i) rec.indices[i] = idx[i];
        rec.lightingControl = be16(base + p + 0);
        rec.cmdCtrl = be16(base + p + 2);
        rec.cmdPmod = be16(base + p + 4);
        rec.cmdColr = be16(base + p + 6);
        rec.cmdSrca = be16(base + p + 8);
        rec.cmdSize = be16(base + p + 10);
        rec.model = modelNumber;
        rec.polygonInModel = localPoly;
        p += 12u;

        const unsigned lm = rec.lightingMode();
        if (lm == 1u) {
            if (p + 8u > mcb.size()) return false;
            rec.lighting[0].normal[0] =
                static_cast<std::int16_t>(be16(base + p + 0));
            rec.lighting[0].normal[1] =
                static_cast<std::int16_t>(be16(base + p + 2));
            rec.lighting[0].normal[2] =
                static_cast<std::int16_t>(be16(base + p + 4));
            rec.lightingCount = 1;
            p += 8u;
        } else if (lm == 2u) {
            if (p + 48u > mcb.size()) return false;
            for (unsigned c = 0; c < 4; ++c) {
                const auto q = p + c * 12u;
                rec.lighting[c].normal[0] =
                    static_cast<std::int16_t>(be16(base + q + 0));
                rec.lighting[c].normal[1] =
                    static_cast<std::int16_t>(be16(base + q + 2));
                rec.lighting[c].normal[2] =
                    static_cast<std::int16_t>(be16(base + q + 4));
                rec.lighting[c].color[0] = be16(base + q + 6);
                rec.lighting[c].color[1] = be16(base + q + 8);
                rec.lighting[c].color[2] = be16(base + q + 10);
                rec.lighting[c].hasColor = true;
            }
            rec.lightingCount = 4;
            p += 48u;
        } else if (lm == 3u) {
            if (p + 24u > mcb.size()) return false;
            for (unsigned c = 0; c < 4; ++c) {
                const auto q = p + c * 6u;
                rec.lighting[c].normal[0] =
                    static_cast<std::int16_t>(be16(base + q + 0));
                rec.lighting[c].normal[1] =
                    static_cast<std::int16_t>(be16(base + q + 2));
                rec.lighting[c].normal[2] =
                    static_cast<std::int16_t>(be16(base + q + 4));
            }
            rec.lightingCount = 4;
            p += 24u;
        }

        std::uint8_t cr, cg, cb;
        debugColor(localPoly, modelNumber, cr, cg, cb);
        constexpr unsigned tc[6] = {0,1,2,0,2,3};
        for (unsigned k = 0; k < 6; ++k) {
            const V3& v = verts[idx[tc[k]]];
            out.vertices.push_back({v.x,v.y,v.z,cr,cg,cb,255});
            out.lightingVertices.push_back({v.x,v.y,v.z,255,255,255,255});
        }
        out.polygonRecords.push_back(rec);
        out.gouraud555.push_back({});
        ++out.polygons;
        ++localPoly;
    }

    ++out.models;
    return true;
}

static bool traverse(const std::vector<std::uint8_t>& mcb,
                     std::uint32_t node,
                     std::uint32_t poseBase,
                     unsigned& bone,
                     const M4& parent,
                     BasicWingDebugMesh& out,
                     unsigned depth)
{
    if (!node) return true;
    if (depth > 128u || node + 12u > mcb.size()) return false;

    do {
        Pose pose{};
        if (!readPose(mcb, poseBase, bone, pose))
            return false;
        const M4 world = mul(parent, poseMatrix(pose));

        const auto model = be32(mcb.data() + node + 0);
        const auto child = be32(mcb.data() + node + 4);
        const auto next  = be32(mcb.data() + node + 8);

        if (model && !appendModel(mcb, model, world, out.models, out))
            return false;

        ++bone;
        if (child && !traverse(
                mcb, child, poseBase, bone, world, out, depth + 1u))
            return false;
        node = next;
    } while (node);

    return true;
}

static std::uint32_t rgb555(std::uint16_t c)
{
    const std::uint32_t r5 = c & 31u;
    const std::uint32_t g5 = (c >> 5) & 31u;
    const std::uint32_t b5 = (c >> 10) & 31u;
    return 0xFF000000u |
           ((r5 << 3) | (r5 >> 2)) |
           (((g5 << 3) | (g5 >> 2)) << 8) |
           (((b5 << 3) | (b5 >> 2)) << 16);
}

static bool decodeTextures(const std::vector<std::uint8_t>& cgb,
                           BasicWingDebugMesh& out)
{
    out.decodedTextureData.clear();
    out.polygonTextureIndices.assign(
        out.polygonRecords.size(), 0xFFFFu);
    out.uniqueTextures = 0;
    out.decodedTextures = 0;
    out.indirectCramPixels = 0;

    bool allValid = true;
    for (std::size_t i = 0; i < out.polygonRecords.size(); ++i) {
        const auto& rec = out.polygonRecords[i];

        int existing = -1;
        for (std::size_t t = 0; t < out.decodedTextureData.size(); ++t) {
            const auto& tex = out.decodedTextureData[t];
            if (tex.cmdPmod == rec.cmdPmod &&
                tex.cmdColr == rec.cmdColr &&
                tex.cmdSrca == rec.cmdSrca &&
                tex.cmdSize == rec.cmdSize) {
                existing = static_cast<int>(t);
                break;
            }
        }
        if (existing >= 0) {
            out.polygonTextureIndices[i] =
                static_cast<std::uint16_t>(existing);
            continue;
        }

        ++out.uniqueTextures;
        const unsigned w = rec.textureWidth();
        const unsigned h = rec.textureHeight();
        const unsigned addr = rec.textureByteAddress();
        if (!w || !h) { allValid = false; continue; }

        DecodedMode1Texture tex{};
        tex.cmdPmod = rec.cmdPmod;
        tex.cmdColr = rec.cmdColr;
        tex.cmdSrca = rec.cmdSrca;
        tex.cmdSize = rec.cmdSize;
        tex.width = w;
        tex.height = h;
        tex.rgba.assign(w * h, 0u);

        const unsigned mode = rec.colorMode();
        if (mode == 1u) {
            const unsigned lut = static_cast<unsigned>(rec.cmdColr) << 3;
            const unsigned bytes = (w * h) / 2u;
            if (static_cast<std::size_t>(addr) + bytes > cgb.size() ||
                static_cast<std::size_t>(lut) + 32u > cgb.size()) {
                allValid = false; continue;
            }

            const bool spd = (rec.cmdPmod & 0x40u) != 0;
            const bool endDisabled = (rec.cmdPmod & 0x80u) != 0;
            const bool endMode = (rec.cmdPmod & 0x20u) == 0;
            unsigned pixel = 0;
            for (unsigned y = 0; y < h; ++y) {
                unsigned endCount = 0;
                for (unsigned x = 0; x < w; ++x, ++pixel) {
                    const std::uint8_t packed =
                        cgb[addr + (x + y * w) / 2u];
                    const std::uint8_t dot =
                        (x & 1u) ? (packed & 15u) : (packed >> 4);
                    if ((endMode && endCount >= 2u) ||
                        (dot == 0u && !spd))
                        continue;
                    if (dot == 15u && !endDisabled) {
                        ++endCount;
                        continue;
                    }
                    const std::uint16_t color =
                        be16(cgb.data() + lut + dot * 2u);
                    if (color & 0x8000u)
                        tex.rgba[pixel] = rgb555(color);
                    else if (color)
                        ++out.indirectCramPixels;
                }
            }
        } else if (mode == 5u) {
            const unsigned bytes = w * h * 2u;
            if (static_cast<std::size_t>(addr) + bytes > cgb.size()) {
                allValid = false; continue;
            }
            for (unsigned p = 0; p < w * h; ++p) {
                const auto color = be16(cgb.data() + addr + p * 2u);
                if (color & 0x8000u)
                    tex.rgba[p] = rgb555(color);
            }
        } else {
            allValid = false;
            continue;
        }

        const auto index =
            static_cast<std::uint16_t>(out.decodedTextureData.size());
        out.decodedTextureData.push_back(std::move(tex));
        out.polygonTextureIndices[i] = index;
        ++out.decodedTextures;
    }

    out.mode1DecodeValid =
        allValid &&
        out.decodedTextures == out.uniqueTextures;
    out.mode1DecodeFullyResolved =
        out.mode1DecodeValid &&
        out.indirectCramPixels == 0;
    out.cgbReferencesValid = out.mode1DecodeValid;

    return out.mode1DecodeFullyResolved;
}

} // namespace

namespace lagi::azel {

bool build_edge_idle_debug_mesh(BasicWingDebugMesh& out)
{
    out = {};

    sSaturnMemoryFile* overlay = town_overlay_file();
    const TownBootstrapInfo& info = town_bootstrap_info();
    if (!overlay || !overlay->m_data || !info.edgeEA ||
        info.edgeEA < overlay->m_base)
        return false;

    const std::uint32_t edge =
        info.edgeEA - overlay->m_base;
    if (edge + 0x26u > overlay->m_dataSize)
        return false;

    const auto* def = overlay->m_data + edge;
    const std::uint16_t hierarchyIndex = be16(def + 0x22u);
    const std::uint16_t poseIndex = be16(def + 0x24u);

    std::vector<std::uint8_t> mcb;
    std::vector<std::uint8_t> cgb;
    if (!lagi::disc::read_file("COMMON3.MCB", mcb) ||
        !lagi::disc::read_file("COMMON3.CGB", cgb) ||
        mcb.size() < 16u)
        return false;

    if (static_cast<std::size_t>(hierarchyIndex) + 4u > mcb.size() ||
        static_cast<std::size_t>(poseIndex) + 4u > mcb.size())
        return false;

    const std::uint32_t hierarchy =
        be32(mcb.data() + hierarchyIndex);
    const std::uint32_t pose =
        be32(mcb.data() + poseIndex);
    if (!hierarchy || !pose)
        return false;

    unsigned bones = 0;
    if (!traverse(mcb, hierarchy, pose, bones, identity(), out, 0u))
        return false;

    out.cgbBytes = static_cast<unsigned>(cgb.size());
    const bool textures = decodeTextures(cgb, out);

    lagi::platform::logging::writef(
        "[Edge] COMMON3 idle: hierarchyIndex=0x%X poseIndex=0x%X bones=%u models=%u polys=%u textures=%u/%u indirect=%u %s\\n",
        hierarchyIndex, poseIndex, bones, out.models, out.polygons,
        out.decodedTextures, out.uniqueTextures,
        out.indirectCramPixels,
        textures ? "textures ready" : "texture decode incomplete");

    return bones != 0u &&
           out.models != 0u &&
           out.polygons != 0u &&
           out.vertices.size() == out.polygons * 6u &&
           out.lightingVertices.size() == out.vertices.size() &&
           out.polygonRecords.size() == out.polygons &&
           out.gouraud555.size() == out.polygons &&
           textures;
}

} // namespace lagi::azel
