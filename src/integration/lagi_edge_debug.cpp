#include "lagi/debug_mesh.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/azel_town_runtime.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

#include <algorithm>
#include <array>
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
    std::int32_t sx, sy, sz;
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
    out.sx = bes32(b + 24); out.sy = bes32(b + 28); out.sz = bes32(b + 32);
    return true;
}

static M4 scale(float x, float y, float z)
{
    M4 r = identity();
    r.m[0] = x; r.m[5] = y; r.m[10] = z;
    return r;
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
    r = mul(r, scale(
        static_cast<float>(p.sx) / 65536.0f,
        static_cast<float>(p.sy) / 65536.0f,
        static_cast<float>(p.sz) / 65536.0f));
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
                     unsigned depth,
                     const std::vector<Pose>* poseOverride = nullptr)
{
    if (!node) return true;
    if (depth > 128u || node + 12u > mcb.size()) return false;

    do {
        Pose pose{};
        if (poseOverride) {
            if (bone >= poseOverride->size()) return false;
            pose = (*poseOverride)[bone];
        } else if (!readPose(mcb, poseBase, bone, pose)) return false;
        const M4 world = mul(parent, poseMatrix(pose));

        const auto model = be32(mcb.data() + node + 0);
        const auto child = be32(mcb.data() + node + 4);
        const auto next  = be32(mcb.data() + node + 8);

        if (model && !appendModel(mcb, model, world, out.models, out))
            return false;

        ++bone;
        if (child && !traverse(
                mcb, child, poseBase, bone, world, out, depth + 1u,
                poseOverride))
            return false;
        node = next;
    } while (node);

    return true;
}

struct RawAnimationTrack {
    std::int16_t length[9]{};
    std::vector<std::int16_t> data[9];
};

struct RawAnimation {
    std::uint16_t flags = 0;
    std::uint16_t bones = 0;
    std::uint16_t frames = 0;
    std::vector<RawAnimationTrack> tracks;
};

struct TrackState {
    unsigned step = 0;
    int delay = 0;
    std::int32_t value = 0;
};

static bool parseAnimation(const std::vector<std::uint8_t>& bundle,
                           std::uint32_t tableOffset,
                           RawAnimation& out)
{
    if (tableOffset + 4u > bundle.size()) return false;
    const std::uint32_t animation = be32(bundle.data() + tableOffset);
    if (!animation || animation + 12u > bundle.size()) return false;
    out = {};
    out.flags = be16(bundle.data() + animation);
    out.bones = be16(bundle.data() + animation + 2u);
    out.frames = be16(bundle.data() + animation + 4u);
    const std::uint32_t headers = be32(bundle.data() + animation + 8u);
    if (!out.flags || !out.bones || !out.frames ||
        out.bones > 128u || out.frames > 1024u) return false;
    out.tracks.resize(out.bones);
    for (unsigned bone = 0; bone < out.bones; ++bone) {
        const std::uint64_t header = static_cast<std::uint64_t>(animation) +
            headers + static_cast<std::uint64_t>(bone) * 0x38u;
        if (header + 0x38u > bundle.size()) return false;
        auto& track = out.tracks[bone];
        for (unsigned channel = 0; channel < 9u; ++channel)
            track.length[channel] = static_cast<std::int16_t>(
                be16(bundle.data() + header + channel * 2u));
        for (unsigned channel = 0; channel < 9u; ++channel) {
            const int length = track.length[channel];
            if (length <= 0) continue;
            const std::uint32_t offset = be32(
                bundle.data() + header + 0x14u + channel * 4u);
            const std::uint64_t start =
                static_cast<std::uint64_t>(animation) + offset;
            if (start + static_cast<std::uint64_t>(length) * 2u >
                bundle.size()) return false;
            track.data[channel].resize(static_cast<std::size_t>(length));
            for (int i = 0; i < length; ++i)
                track.data[channel][static_cast<std::size_t>(i)] =
                    static_cast<std::int16_t>(be16(
                        bundle.data() + start + static_cast<unsigned>(i) * 2u));
        }
    }
    return true;
}

static std::int32_t stepTrack(
    TrackState& state, const std::vector<std::int16_t>& data)
{
    if (data.empty()) return 0;
    if (state.delay > 0) {
        --state.delay;
        return state.value;
    }
    if (state.step) {
        const std::uint16_t packed =
            static_cast<std::uint16_t>(data[state.step]);
        state.delay = static_cast<int>(packed & 0xFu) - 1;
        state.value = static_cast<std::int16_t>(packed & 0xFFF0u);
    } else {
        state.delay = 0;
        state.value = static_cast<std::int32_t>(data[0]) * 16;
    }
    state.step = (state.step + 1u) % data.size();
    return state.value;
}

static bool buildAnimationClip(const std::vector<std::uint8_t>& bundle,
                               std::uint32_t animationIndex,
                               std::uint32_t hierarchy,
                               std::uint32_t staticPoseOffset,
                               unsigned hierarchyBones,
                               EdgeAnimationClip& clip)
{
    RawAnimation animation{};
    if (!parseAnimation(bundle, animationIndex, animation) ||
        animation.bones != hierarchyBones) return false;
    const unsigned type = animation.flags & 7u;
    if (type != 0u && type != 1u && type != 3u &&
        type != 4u && type != 5u) return false;

    std::vector<Pose> staticPose(animation.bones);
    for (unsigned bone = 0; bone < animation.bones; ++bone)
        if (!readPose(bundle, staticPoseOffset, bone, staticPose[bone]))
            return false;
    std::vector<Pose> pose = staticPose;
    std::vector<std::array<TrackState, 9>> trackState(animation.bones);
    std::vector<std::array<std::int32_t, 9>> fractional(animation.bones);
    clip = {};
    clip.flags = animation.flags;
    clip.frames.reserve(animation.frames);

    auto channelValue = [](Pose& p, unsigned channel) -> std::int32_t& {
        switch (channel) {
        case 0: return p.tx; case 1: return p.ty; case 2: return p.tz;
        case 3: return p.rx; case 4: return p.ry; case 5: return p.rz;
        case 6: return p.sx; case 7: return p.sy; default: return p.sz;
        }
    };

    for (unsigned frame = 0; frame < animation.frames; ++frame) {
        for (unsigned bone = 0; bone < animation.bones; ++bone) {
            for (unsigned channel = 0; channel < 9u; ++channel) {
                const unsigned flag = channel < 3u ? 0x8u :
                    (channel < 6u ? 0x10u : 0x20u);
                if (!(animation.flags & flag)) continue;
                const auto& data = animation.tracks[bone].data[channel];
                if (data.empty()) continue;
                std::int32_t& value = channelValue(pose[bone], channel);
                const std::int32_t quant = channel < 3u || channel >= 6u
                    ? 0x10 : 0x10000;
                // Edge initializes its model with flag 0x100. Azel's type
                // 1/4/5 position handlers therefore animate only the root
                // and retain the static root translation on frame zero.
                if (channel < 3u &&
                    (type == 1u || type == 4u || type == 5u)) {
                    if (bone != 0u) continue;
                    if (type == 1u) {
                        const std::int32_t delta = stepTrack(
                            trackState[bone][channel], data);
                        if (frame != 0u) value += delta;
                    } else {
                        const unsigned divisor = type == 4u ? 2u : 4u;
                        if (frame == 0u) {
                            stepTrack(trackState[bone][channel], data);
                            fractional[bone][channel] =
                                animation.frames > 1u
                                ? stepTrack(trackState[bone][channel], data) /
                                      static_cast<std::int32_t>(divisor)
                                : 0;
                        } else {
                            value += fractional[bone][channel];
                            if ((frame % divisor) == 0u)
                                fractional[bone][channel] =
                                    frame + 1u < animation.frames
                                    ? stepTrack(
                                          trackState[bone][channel], data) /
                                          static_cast<std::int32_t>(divisor)
                                    : 0;
                        }
                    }
                    continue;
                }
                if (type == 0u) {
                    if (frame >= data.size()) return false;
                    value = static_cast<std::int32_t>(data[frame]) * quant;
                } else if (type == 3u) {
                    if ((frame & 3u) == 0u) {
                        const unsigned key = frame / 4u;
                        if (key >= data.size()) return false;
                        value = static_cast<std::int32_t>(data[key]) * quant;
                        fractional[bone][channel] =
                            key + 1u < data.size()
                            ? (static_cast<std::int32_t>(data[key + 1u] - data[key]) * quant) / 4
                            : 0;
                    } else value += fractional[bone][channel];
                } else if (type == 1u) {
                    const std::int32_t delta = stepTrack(
                        trackState[bone][channel], data) *
                        (channel < 3u || channel >= 6u ? 1 : 0x1000);
                    if (frame == 0u) value = delta;
                    else value += delta;
                } else {
                    const unsigned divisor = type == 4u ? 2u : 4u;
                    if ((frame % divisor) == 0u) {
                        if (frame == 0u)
                            value = stepTrack(trackState[bone][channel], data) *
                                (channel < 3u || channel >= 6u ? 1 : 0x1000);
                        else value += fractional[bone][channel];
                        fractional[bone][channel] =
                            frame + 1u < animation.frames
                            ? (stepTrack(trackState[bone][channel], data) *
                               (channel < 3u || channel >= 6u ? 1 : 0x1000)) /
                                  static_cast<std::int32_t>(divisor)
                            : 0;
                    } else value += fractional[bone][channel];
                }
            }
        }
        BasicWingDebugMesh frameMesh{};
        unsigned bone = 0;
        if (!traverse(bundle, hierarchy, staticPoseOffset, bone, identity(),
                      frameMesh, 0u, &pose) || bone != hierarchyBones)
            return false;
        BasicWingAnimationFrame output{};
        output.vertices = std::move(frameMesh.vertices);
        output.lightingNormals = std::move(frameMesh.lightingNormals);
        clip.frames.push_back(std::move(output));
    }
    clip.valid = !clip.frames.empty();
    return clip.valid;
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
    unsigned fallbackTextures = 0;
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

        DecodedMode1Texture tex{};
        tex.cmdPmod = rec.cmdPmod;
        tex.cmdColr = rec.cmdColr;
        tex.cmdSrca = rec.cmdSrca;
        tex.cmdSize = rec.cmdSize;
        tex.width = w;
        tex.height = h;

        const unsigned mode = rec.colorMode();

        // Edge's COMMON3 hierarchy contains one command descriptor that is
        // not a normal textured sprite. Keep the playable-room bring-up alive
        // with an obvious 1x1 diagnostic texel instead of rejecting the entire
        // actor. The log records the exact command so we can implement its
        // native VDP1 semantics once orientation/placement is verified.
        if (!w || !h || (mode != 1u && mode != 5u)) {
            tex.width = 1u;
            tex.height = 1u;
            tex.rgba.assign(1u, 0xFFFF00FFu);
            ++fallbackTextures;

            lagi::platform::logging::writef(
                "[Edge] texture fallback poly=%u CTRL=%04X PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X mode=%u %ux%u\n",
                static_cast<unsigned>(i),
                rec.cmdCtrl, rec.cmdPmod, rec.cmdColr,
                rec.cmdSrca, rec.cmdSize, mode, w, h);
        } else {
            tex.rgba.assign(w * h, 0u);
        }

        if (!w || !h || (mode != 1u && mode != 5u)) {
            // Diagnostic texel already populated above.
        } else if (mode == 1u) {
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

    lagi::platform::logging::writef(
        "[Edge] texture decode complete: %u/%u descriptors, fallback=%u, indirect=%u\n",
        out.decodedTextures, out.uniqueTextures,
        fallbackTextures, out.indirectCramPixels);

    return out.mode1DecodeFullyResolved;
}

} // namespace

namespace lagi::azel {

bool build_edge_idle_debug_mesh(BasicWingDebugMesh& out)
{
    out = {};

    sSaturnMemoryFile* overlay = town_overlay_file();
    const TownRuntimeState& runtime = town_runtime();
    if (!overlay || !overlay->m_data ||
        !runtime.initialized || !runtime.edgeEA ||
        runtime.edgeEA < overlay->m_base)
        return false;

    const std::uint32_t edge =
        runtime.edgeEA - overlay->m_base;
    if (edge + 0x30u > overlay->m_dataSize)
        return false;

    const auto* def = overlay->m_data + edge;
    const std::uint16_t hierarchyIndex = be16(def + 0x22u);
    const std::uint16_t poseIndex = be16(def + 0x24u);

    const std::vector<std::uint8_t>* mcbOwned =
        town_runtime_resource("COMMON3.MCB");
    const std::vector<std::uint8_t>* cgbOwned =
        town_runtime_resource("COMMON3.CGB");
    if (!mcbOwned || !cgbOwned || mcbOwned->size() < 16u)
        return false;

    const std::vector<std::uint8_t>& mcb = *mcbOwned;
    const std::vector<std::uint8_t>& cgb = *cgbOwned;

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

    out.edgeAnimationClips.resize(9u);
    const std::uint32_t animationTableEA = be32(def + 0x2Cu);
    unsigned decodedClips = 0;
    if (animationTableEA >= overlay->m_base) {
        const std::uint32_t animationTable =
            animationTableEA - overlay->m_base;
        for (unsigned animationId = 0; animationId < 9u; ++animationId) {
            const std::uint32_t entry = animationTable + animationId * 4u;
            if (entry + 4u > overlay->m_dataSize) break;
            const std::uint16_t bundleSelector =
                be16(overlay->m_data + entry);
            const std::uint16_t animationIndex =
                be16(overlay->m_data + entry + 2u);
            if (!animationIndex) continue;
            if (buildAnimationClip(
                    mcb, animationIndex, hierarchy, pose, bones,
                    out.edgeAnimationClips[animationId])) {
                ++decodedClips;
                lagi::platform::logging::writef(
                    "[Edge] animation %u selector=%u index=%04X flags=%04X frames=%u\n",
                    animationId, bundleSelector, animationIndex,
                    out.edgeAnimationClips[animationId].flags,
                    static_cast<unsigned>(
                        out.edgeAnimationClips[animationId].frames.size()));
            }
        }
    }

    out.cgbBytes = static_cast<unsigned>(cgb.size());
    const bool textures = decodeTextures(cgb, out);

    lagi::platform::logging::writef(
        "[Edge] COMMON3 actor: hierarchyIndex=0x%X poseIndex=0x%X bones=%u models=%u polys=%u animations=%u textures=%u/%u indirect=%u %s\n",
        hierarchyIndex, poseIndex, bones, out.models, out.polygons,
        decodedClips,
        out.decodedTextures, out.uniqueTextures,
        out.indirectCramPixels,
        textures ? "textures ready" : "texture decode incomplete");

    const bool coreAnimations =
        out.edgeAnimationClips[1].valid &&
        out.edgeAnimationClips[2].valid &&
        out.edgeAnimationClips[4].valid;
    const bool ambientAnimation =
        out.edgeAnimationClips[5].valid ||
        out.edgeAnimationClips[6].valid ||
        out.edgeAnimationClips[7].valid ||
        out.edgeAnimationClips[8].valid;

    return bones != 0u &&
           out.models != 0u &&
           out.polygons != 0u &&
           out.vertices.size() == out.polygons * 6u &&
           out.lightingVertices.size() == out.vertices.size() &&
           out.polygonRecords.size() == out.polygons &&
           out.gouraud555.size() == out.polygons &&
           coreAnimations && ambientAnimation &&
           textures;
}

bool build_edge_shadow_debug_mesh(BasicWingDebugMesh& out)
{
    out = {};

    sSaturnMemoryFile* overlay = town_overlay_file();
    const TownRuntimeState& runtime = town_runtime();
    if (!overlay || !overlay->m_data ||
        !runtime.initialized || !runtime.edgeEA ||
        runtime.edgeEA < overlay->m_base)
        return false;

    const std::uint32_t edge = runtime.edgeEA - overlay->m_base;
    if (edge + 0x30u > overlay->m_dataSize)
        return false;

    const auto* def = overlay->m_data + edge;
    const std::uint32_t animationTableEA = be32(def + 0x2Cu);
    if (animationTableEA < overlay->m_base)
        return false;
    const std::uint32_t animationTable =
        animationTableEA - overlay->m_base;
    if (animationTable + 4u > overlay->m_dataSize)
        return false;

    // sEdgeTask::Draw uses the second halfword of animation-table entry 0 as
    // the COMMON3 shadow model key.
    const std::uint16_t shadowKey =
        be16(overlay->m_data + animationTable + 2u);

    const std::vector<std::uint8_t>* mcbOwned =
        town_runtime_resource("COMMON3.MCB");
    if (!mcbOwned || shadowKey + 4u > mcbOwned->size())
        return false;
    const auto& mcb = *mcbOwned;

    // File-bundle model keys point at a BE32 raw model offset.
    const std::uint32_t modelOffset =
        be32(mcb.data() + shadowKey);
    if (!modelOffset ||
        !appendModel(mcb, modelOffset, identity(), 0u, out))
        return false;

    // The Saturn shadow is a normal textured VDP1 model with mesh mode set.
    // Its texture supplies the oval alpha silhouette; CMDPMOD mesh supplies
    // the alternating destination-pixel coverage. Do not replace the material
    // with a solid marker texture.
    for (auto& record : out.polygonRecords)
        record.cmdPmod |= 0x0100u;

    const std::vector<std::uint8_t>* cgbOwned =
        town_runtime_resource("COMMON3.CGB");
    if (!cgbOwned)
        return false;
    const bool textures = decodeTextures(*cgbOwned, out);
    if (!textures)
        return false;

    lagi::platform::logging::writef(
        "[Edge] shadow model key=%04X raw=%08X polys=%u textures=%u/%u mesh-stipple\n",
        shadowKey, modelOffset, out.polygons,
        out.decodedTextures, out.uniqueTextures);

    return out.polygons != 0u &&
           out.vertices.size() == out.polygons * 6u &&
           out.polygonRecords.size() == out.polygons &&
           out.polygonTextureIndices.size() == out.polygons &&
           out.mode1DecodeFullyResolved;
}

} // namespace lagi::azel
