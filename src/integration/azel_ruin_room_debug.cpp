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

static std::array<float,3> applyMat3(
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

static s32 bes32(const std::vector<u8>& data, u32 offset)
{
    return static_cast<s32>(be32(data, offset));
}

static bool recoverInitialRuinCamera(
    sSaturnMemoryFile* overlay,
    StaticRoomDebugMesh& out)
{
    if (!overlay || !overlay->m_data)
        return false;

    constexpr u32 kEdgeEA = 0x0605E990u;
    constexpr u32 kScriptEA = 0x06054398u;
    constexpr u32 kScanBytes = 0x800u;
    constexpr u32 kSetNpcLocationEA = 0x0605AEE0u;
    constexpr u32 kSetNpcOrientationEA = 0x0605AF0Eu;
    constexpr u32 kSetupCameraFollowEA = 0x06057058u;

    if (kEdgeEA < overlay->m_base ||
        kScriptEA < overlay->m_base)
        return false;

    const u32 edgeOffset = kEdgeEA - overlay->m_base;
    if (edgeOffset + 0x20u > overlay->m_dataSize)
        return false;

    const std::vector<u8> view(
        overlay->m_data,
        overlay->m_data + overlay->m_dataSize);

    s32 edgePosRaw[3] = {
        bes32(view, edgeOffset + 0x08u),
        bes32(view, edgeOffset + 0x0Cu),
        bes32(view, edgeOffset + 0x10u)
    };
    s32 edgeRotRaw[3] = {
        bes32(view, edgeOffset + 0x14u),
        bes32(view, edgeOffset + 0x18u),
        bes32(view, edgeOffset + 0x1Cu)
    };

    bool sawScriptLocation = false;
    bool sawScriptOrientation = false;
    bool sawSetupCameraFollow = false;

    const u32 scriptStart = kScriptEA - overlay->m_base;
    const u32 scriptEnd = std::min<u32>(
        overlay->m_dataSize,
        scriptStart + kScanBytes);

    // Replay the camera-relevant native calls in script order up to the
    // setupCameraFollowMode() call. This mirrors the startup state actually
    // visible to scriptFunction_6057058_sub0Sub0(), instead of using the raw
    // NPC definition before the script has repositioned Edge.
    for (u32 p = scriptStart; p + 8u <= scriptEnd; ++p) {
        if (view[p] != 7u)
            continue;

        const u8 argc = view[p + 1u];
        if (argc > 4u)
            continue;

        u32 callData = p + 2u;
        callData = (callData + 3u) & ~3u;

        if (callData + 4u +
                static_cast<u32>(argc) * 4u >
            overlay->m_dataSize)
            continue;

        const u32 functionEA = be32(view, callData);

        if (functionEA == kSetNpcLocationEA &&
            argc == 4u) {
            const s32 npcIndex =
                bes32(view, callData + 4u);
            if (npcIndex == 0) {
                edgePosRaw[0] =
                    bes32(view, callData + 8u);
                edgePosRaw[1] =
                    bes32(view, callData + 12u);
                edgePosRaw[2] =
                    bes32(view, callData + 16u);
                sawScriptLocation = true;
            }
        } else if (
            functionEA == kSetNpcOrientationEA &&
            argc == 4u) {
            const s32 npcIndex =
                bes32(view, callData + 4u);
            if (npcIndex == 0) {
                edgeRotRaw[0] =
                    bes32(view, callData + 8u);
                edgeRotRaw[1] =
                    bes32(view, callData + 12u);
                edgeRotRaw[2] =
                    bes32(view, callData + 16u);
                sawScriptOrientation = true;
            }
        } else if (
            functionEA == kSetupCameraFollowEA &&
            argc == 0u) {
            sawSetupCameraFollow = true;
            break;
        }
    }

    if (!sawSetupCameraFollow) {
        lagi::platform::logging::writef(
            "[RoomDebug] setupCameraFollowMode not found while recovering initial camera\n");
        return false;
    }

    const std::array<float,3> edgePos = {{
        static_cast<float>(edgePosRaw[0]) / 65536.0f,
        static_cast<float>(edgePosRaw[1]) / 65536.0f,
        static_cast<float>(edgePosRaw[2]) / 65536.0f
    }};

    const std::int16_t edgeRotX =
        static_cast<std::int16_t>(
            (edgeRotRaw[0] >> 16) & 0x0FFF);
    const std::int16_t edgeRotY =
        static_cast<std::int16_t>(
            (edgeRotRaw[1] >> 16) & 0x0FFF);

    // scriptFunction_6057058_sub0Sub0():
    // m18_position = Edge position + (0, 0x1800, 0)
    // camera basis = rotate Y, then X
    // raw camera = focus + basis.Z * 0x199
    // target     = focus + basis.Z * -0x1000
    std::array<float,3> focus = edgePos;
    focus[1] += 0x1800 / 65536.0f;

    const Mat3 cameraBasis =
        mul3(rotY(edgeRotY), rotX(edgeRotX));
    const std::array<float,3> forward = {{
        cameraBasis.m[0][2],
        cameraBasis.m[1][2],
        cameraBasis.m[2][2]
    }};

    constexpr float kCameraOffset =
        0x199 / 65536.0f;
    constexpr float kTargetOffset =
        -0x1000 / 65536.0f;

    for (unsigned int i = 0; i < 3; ++i) {
        out.cameraPosition[i] =
            focus[i] + forward[i] * kCameraOffset;
        out.cameraTarget[i] =
            focus[i] + forward[i] * kTargetOffset;
        out.cameraUp[i] = out.cameraPosition[i];
    }
    out.cameraUp[1] += 1.0f;

    out.cameraFovDegrees = 80.0f;
    out.cameraNear = 0x800 / 65536.0f;
    out.cameraFar = 0xF000 / 65536.0f;
    out.cameraValid = true;

    lagi::platform::logging::writef(
        "[RoomDebug] initial camera edgePos=(%.5f,%.5f,%.5f) rot=(%03X,%03X) scriptedPos=%s scriptedRot=%s cam=(%.5f,%.5f,%.5f) target=(%.5f,%.5f,%.5f) fov=%.1f near=%.5f far=%.5f\n",
        edgePos[0], edgePos[1], edgePos[2],
        static_cast<unsigned>(edgeRotX) & 0xFFFu,
        static_cast<unsigned>(edgeRotY) & 0xFFFu,
        sawScriptLocation ? "yes" : "no",
        sawScriptOrientation ? "yes" : "no",
        out.cameraPosition[0],
        out.cameraPosition[1],
        out.cameraPosition[2],
        out.cameraTarget[0],
        out.cameraTarget[1],
        out.cameraTarget[2],
        out.cameraFovDegrees,
        out.cameraNear,
        out.cameraFar);

    return true;
}

static void traceInitialRuinScriptNativeCalls(
    sSaturnMemoryFile* overlay)
{
    if (!overlay || !overlay->m_data)
        return;

    constexpr u32 kScriptEA = 0x06054398u;
    constexpr u32 kScanBytes = 0x800u;
    constexpr u32 kSetNpcLocationEA = 0x0605AEE0u;
    constexpr u32 kSetNpcOrientationEA = 0x0605AF0Eu;
    constexpr u32 kSetupCameraFollowEA = 0x06057058u;
    constexpr u32 kTownCameraSetupEA = 0x0605C55Cu;

    if (kScriptEA < overlay->m_base)
        return;

    const u32 start = kScriptEA - overlay->m_base;
    const u32 end = std::min<u32>(
        overlay->m_dataSize,
        start + kScanBytes);

    const std::vector<u8> view(
        overlay->m_data,
        overlay->m_data + overlay->m_dataSize);

    for (u32 p = start; p + 8u <= end; ++p) {
        if (view[p] != 7u)
            continue;

        const u8 argc = view[p + 1u];
        if (argc > 4u)
            continue;

        u32 callData = p + 2u;
        callData = (callData + 3u) & ~3u;
        if (callData + 4u + static_cast<u32>(argc) * 4u >
            overlay->m_dataSize)
            continue;

        const u32 functionEA = be32(view, callData);
        if (functionEA != kSetNpcLocationEA &&
            functionEA != kSetNpcOrientationEA &&
            functionEA != kSetupCameraFollowEA &&
            functionEA != kTownCameraSetupEA)
            continue;

        s32 args[4]{};
        for (unsigned int a = 0; a < argc; ++a)
            args[a] = bes32(view, callData + 4u + a * 4u);

        lagi::platform::logging::writef(
            "[RoomDebug] script native @%08X fn=%08X argc=%u args=%08X/%08X/%08X/%08X\n",
            overlay->m_base + p,
            functionEA,
            static_cast<unsigned>(argc),
            static_cast<unsigned>(args[0]),
            static_cast<unsigned>(args[1]),
            static_cast<unsigned>(args[2]),
            static_cast<unsigned>(args[3]));
    }
}

static bool findInitialRuinSceneLight(
    sSaturnMemoryFile* overlay,
    StaticRoomDebugMesh& out)
{
    if (!overlay || !overlay->m_data || overlay->m_dataSize < 16u)
        return false;

    constexpr u32 kInitialScriptEA = 0x06054398u;
    constexpr u32 kTownCameraSetupEA = 0x0605C55Cu;

    if (kInitialScriptEA < overlay->m_base)
        return false;

    const u32 start = kInitialScriptEA - overlay->m_base;
    const u32 end = std::min<u32>(
        overlay->m_dataSize,
        start + 0x1000u);

    const std::vector<u8> view(
        overlay->m_data,
        overlay->m_data + overlay->m_dataSize);

    for (u32 p = start; p + 12u <= end; ++p) {
        if (be32(view, p) != kTownCameraSetupEA)
            continue;

        const u32 angleEA = be32(view, p + 4u);
        const u32 colorEA = be32(view, p + 8u);

        if (angleEA < overlay->m_base ||
            colorEA < overlay->m_base)
            continue;

        const u32 angleOffset = angleEA - overlay->m_base;
        const u32 colorOffset = colorEA - overlay->m_base;
        if (angleOffset + 12u > overlay->m_dataSize ||
            colorOffset + 12u > overlay->m_dataSize)
            continue;

        const s32 rawX = bes32(view, angleOffset + 0u);
        const s32 rawY = bes32(view, angleOffset + 4u);

        const std::int16_t angleX =
            static_cast<std::int16_t>((rawX >> 16) & 0x0FFF);
        const std::int16_t angleY =
            static_cast<std::int16_t>((rawY >> 16) & 0x0FFF);

        // townCamera_setup(): identity, rotate Y, rotate X, use column 2.
        const Mat3 lightMatrix =
            mul3(rotY(angleY), rotX(angleX));

        float lx = lightMatrix.m[0][2];
        float ly = lightMatrix.m[1][2];
        float lz = lightMatrix.m[2][2];
        const float len =
            std::sqrt(lx*lx + ly*ly + lz*lz);
        if (len <= 0.000001f)
            continue;

        out.lightDirection[0] = lx / len;
        out.lightDirection[1] = ly / len;
        out.lightDirection[2] = lz / len;

        out.lightColor[0] = view[colorOffset + 0u];
        out.lightColor[1] = view[colorOffset + 1u];
        out.lightColor[2] = view[colorOffset + 2u];

        for (unsigned int i = 0; i < 3; ++i) {
            const u32 q = colorOffset + 3u + i * 3u;
            out.lightFalloff[i] =
                static_cast<u32>(view[q + 0u]) |
                (static_cast<u32>(view[q + 1u]) << 8) |
                (static_cast<u32>(view[q + 2u]) << 16);
        }

        out.lightingValid = true;

        lagi::platform::logging::writef(
            "[RoomDebug] scene light call=%08X angles=%08X colors=%08X dir=(%.3f,%.3f,%.3f) RGB=%u/%u/%u falloff=%06X/%06X/%06X\n",
            kTownCameraSetupEA,
            angleEA,
            colorEA,
            out.lightDirection[0],
            out.lightDirection[1],
            out.lightDirection[2],
            static_cast<unsigned>(out.lightColor[0]),
            static_cast<unsigned>(out.lightColor[1]),
            static_cast<unsigned>(out.lightColor[2]),
            static_cast<unsigned>(out.lightFalloff[0]),
            static_cast<unsigned>(out.lightFalloff[1]),
            static_cast<unsigned>(out.lightFalloff[2]));

        return true;
    }

    lagi::platform::logging::writef(
        "[RoomDebug] townCamera_setup light call not found in initial script window\n");
    return false;
}

static std::uint32_t rgb555ToRgba8888(u16 color)
{
    const std::uint32_t r5 = color & 0x1Fu;
    const std::uint32_t g5 = (color >> 5) & 0x1Fu;
    const std::uint32_t b5 = (color >> 10) & 0x1Fu;
    const std::uint32_t r8 = (r5 << 3) | (r5 >> 2);
    const std::uint32_t g8 = (g5 << 3) | (g5 >> 2);
    const std::uint32_t b8 = (b5 << 3) | (b5 >> 2);
    return 0xFF000000u | r8 | (g8 << 8) | (b8 << 16);
}

static void decodeRoomTextures(
    const std::vector<u8>& cgb,
    const u8* ruinPalette,
    std::size_t ruinPaletteBytes,
    StaticRoomDebugMesh& out)
{
    out.decodedTextureData.clear();
    out.polygonTextureIndices.assign(
        out.polygonRecords.size(),
        static_cast<std::uint16_t>(0xFFFFu));
    out.uniqueTextures = 0;
    out.decodedTextures = 0;
    out.indirectCramPixels = 0;
    out.texturesValid = false;
    out.texturesFullyResolved = false;

    bool valid = !out.polygonRecords.empty();

    for (std::size_t i = 0; i < out.polygonRecords.size(); ++i) {
        const SaturnPolygonRecord& record = out.polygonRecords[i];

        int existing = -1;
        for (std::size_t t = 0; t < out.decodedTextureData.size(); ++t) {
            const auto& texture = out.decodedTextureData[t];
            if (record.cmdPmod == texture.cmdPmod &&
                record.cmdColr == texture.cmdColr &&
                record.cmdSrca == texture.cmdSrca &&
                record.cmdSize == texture.cmdSize) {
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

        const unsigned commandType = record.cmdCtrl & 0x000Fu;
        const unsigned width = record.textureWidth();
        const unsigned height = record.textureHeight();
        const unsigned colorMode = record.colorMode();
        const unsigned texAddress = record.textureByteAddress();

        // VDP1 command 4 is an untextured polygon. Pinned Azel dispatches it
        // to PolyDrawGL(), where CMDCOLR is used directly as RGB555. Keep the
        // room on one native submission path by representing that flat color
        // as a 1x1 texture/material instead of treating it as a missing
        // texture descriptor.
        if (commandType == 4u) {
            DecodedMode1Texture solid{};
            solid.cmdPmod = record.cmdPmod;
            solid.cmdColr = record.cmdColr;
            solid.cmdSrca = record.cmdSrca;
            solid.cmdSize = record.cmdSize;
            solid.width = 1;
            solid.height = 1;
            solid.rgba.resize(1);

            if (record.cmdColr & 0x8000u) {
                solid.rgba[0] =
                    rgb555ToRgba8888(record.cmdColr);
            } else {
                // Indexed-color command-4 polygons require sprite/CRAM
                // priority-color interpretation. Keep the fallback intact
                // until that case is observed on hardware.
                valid = false;
                continue;
            }

            const std::uint16_t textureIndex =
                static_cast<std::uint16_t>(
                    out.decodedTextureData.size());
            out.decodedTextureData.push_back(std::move(solid));
            out.polygonTextureIndices[i] = textureIndex;
            ++out.decodedTextures;
            continue;
        }

        if (width == 0 || height == 0) {
            valid = false;
            continue;
        }

        DecodedMode1Texture texture{};
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

        if (colorMode == 1) {
            const unsigned texBytes = (width * height) / 2u;
            const unsigned lutAddress =
                static_cast<unsigned>(record.cmdColr) << 3;

            if (static_cast<std::size_t>(texAddress) + texBytes > cgb.size() ||
                static_cast<std::size_t>(lutAddress) + 32u > cgb.size()) {
                valid = false;
                continue;
            }

            unsigned pixel = 0;
            for (unsigned y = 0; y < height; ++y) {
                unsigned endCount = 0;
                for (unsigned x = 0; x < width; ++x, ++pixel) {
                    const unsigned byteOffset =
                        texAddress + (x + y * width) / 2u;
                    const u8 packed = cgb[byteOffset];
                    const u8 dot =
                        (x & 1u) ? (packed & 0x0Fu) : (packed >> 4);

                    if (endMode && endCount >= 2u)
                        continue;
                    if (dot == 0 && !spd)
                        continue;
                    if (dot == 0x0Fu && !endDisabled) {
                        ++endCount;
                        continue;
                    }

                    const u16 lutColor =
                        be16(cgb, lutAddress + dot * 2u);
                    if (lutColor & 0x8000u) {
                        texture.rgba[pixel] =
                            rgb555ToRgba8888(lutColor);
                    } else if (lutColor != 0) {
                        ++out.indirectCramPixels;
                    }
                }
            }
        } else if (colorMode == 0) {
            // Azel ruinBgInit() copies 0x200 bytes from TWN_RUIN:0x0605EBF8
            // to vdp2Palette, which is CRAM byte offset 0xC00. That means
            // palette indices 0x600-0x6FF are backed by this exact overlay
            // palette image.
            const unsigned texBytes = (width * height) / 2u;
            if (static_cast<std::size_t>(texAddress) + texBytes > cgb.size() ||
                !ruinPalette || ruinPaletteBytes < 0x200u) {
                valid = false;
                continue;
            }

            unsigned pixel = 0;
            for (unsigned y = 0; y < height; ++y) {
                unsigned endCount = 0;
                for (unsigned x = 0; x < width; ++x, ++pixel) {
                    const unsigned byteOffset =
                        texAddress + (x + y * width) / 2u;
                    const u8 packed = cgb[byteOffset];
                    const u8 dot =
                        (x & 1u) ? (packed & 0x0Fu) : (packed >> 4);

                    if (endMode && endCount >= 2u)
                        continue;
                    if (dot == 0 && !spd)
                        continue;
                    if (dot == 0x0Fu && !endDisabled) {
                        ++endCount;
                        continue;
                    }

                    const unsigned paletteIndex =
                        static_cast<unsigned>(record.cmdColr) |
                        static_cast<unsigned>(dot);
                    const unsigned paletteByte =
                        paletteIndex * 2u;

                    if (paletteByte < 0xC00u ||
                        paletteByte + 1u >= 0xC00u + ruinPaletteBytes) {
                        valid = false;
                        continue;
                    }

                    const unsigned localPaletteByte =
                        paletteByte - 0xC00u;
                    const u16 color =
                        static_cast<u16>(
                            (static_cast<u16>(ruinPalette[localPaletteByte]) << 8) |
                            static_cast<u16>(ruinPalette[localPaletteByte + 1u]));

                    if (color != 0)
                        texture.rgba[pixel] =
                            rgb555ToRgba8888(color);
                }
            }
        } else if (colorMode == 5) {
            const unsigned texBytes = width * height * 2u;
            if (static_cast<std::size_t>(texAddress) + texBytes > cgb.size()) {
                valid = false;
                continue;
            }

            unsigned pixel = 0;
            for (unsigned y = 0; y < height; ++y) {
                unsigned endCount = 0;
                for (unsigned x = 0; x < width; ++x, ++pixel) {
                    const u16 dot =
                        be16(cgb, texAddress + (x + y * width) * 2u);

                    if (endMode && endCount >= 2u)
                        continue;
                    if (dot == 0 && !spd)
                        continue;
                    if (dot == 0x7FFFu && !endDisabled) {
                        ++endCount;
                        continue;
                    }

                    texture.rgba[pixel] =
                        rgb555ToRgba8888(dot);
                }
            }
        } else {
            // Bank-color modes require live VDP2 CRAM, which is not wired
            // into the reduced room bring-up yet.
            valid = false;
            continue;
        }

        const std::uint16_t textureIndex =
            static_cast<std::uint16_t>(out.decodedTextureData.size());
        out.decodedTextureData.push_back(std::move(texture));
        out.polygonTextureIndices[i] = textureIndex;
        ++out.decodedTextures;
    }

    out.texturesValid =
        valid &&
        out.uniqueTextures != 0 &&
        out.decodedTextures == out.uniqueTextures &&
        out.polygonTextureIndices.size() == out.polygonRecords.size();

    if (out.texturesValid) {
        for (const std::uint16_t index : out.polygonTextureIndices) {
            if (index == 0xFFFFu ||
                index >= out.decodedTextureData.size()) {
                out.texturesValid = false;
                break;
            }
        }
    }

    out.texturesFullyResolved =
        out.texturesValid &&
        out.indirectCramPixels == 0;
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

    // sWorldGridCellTask::Init stores readSaturnVec3(cell), and Draw()
    // translates the current matrix by that cell position before applying
    // each 0x18-byte static object's local translation/rotation.
    const std::array<float,3> cellTranslation = {{
        static_cast<float>(readSaturnS32(cell + 0x00)) / 65536.0f,
        static_cast<float>(readSaturnS32(cell + 0x04)) / 65536.0f,
        static_cast<float>(readSaturnS32(cell + 0x08)) / 65536.0f
    }};

    out.cellOrigin[0] = cellTranslation[0];
    out.cellOrigin[1] = cellTranslation[1];
    out.cellOrigin[2] = cellTranslation[2];

    // initWorldGridData():
    //   gWorldGrid.m2C = MTH_Mul(0x10A3D, cellSize)
    // Both values are 16.16 fixed-point.
    const float cellSize =
        static_cast<float>(info.gridCellSize) / 65536.0f;
    out.cellRadius =
        (static_cast<float>(0x10A3D) / 65536.0f) *
        cellSize;

    // TWN_RUIN never replaces the default town LOD threshold table.
    // resetWorldGrid() therefore leaves:
    //   gWorldGrid.m3C = { 0x7FFFFFFF }
    out.lodDepthCount = 1;
    out.lodDepthThresholds[0] = 0x7FFFFFFF;

    lagi::platform::logging::writef(
        "[RoomDebug] cell origin=(%.5f,%.5f,%.5f) radius=%.5f EA=%08X lodThreshold0=%08X\n",
        cellTranslation[0],
        cellTranslation[1],
        cellTranslation[2],
        out.cellRadius,
        static_cast<unsigned>(cell.m_offset),
        static_cast<unsigned>(out.lodDepthThresholds[0]));

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
    out.objectStates.clear();
    out.objectStates.reserve(64);

    constexpr unsigned int kMaxObjects = 512;
    constexpr unsigned int kMaxVertices = 65520;
    static constexpr unsigned int triCorners[6] = {0,1,2,0,2,3};

    for (unsigned int objectIndex = 0;
         objectIndex < kMaxObjects; ++objectIndex, entry += 0x18) {
        const s32 lodTableEA = readSaturnS32(entry);
        if (lodTableEA == 0)
            break;

        sSaturnPtr lodTable = readSaturnEA(entry);

        StaticRoomObjectState objectState{};
        objectState.firstPolygon =
            static_cast<std::uint32_t>(out.polygonRecords.size());

        const unsigned int lodCount =
            std::min<unsigned int>(
                out.lodDepthCount,
                static_cast<unsigned int>(
                    sizeof(objectState.lodModelOffsets) /
                    sizeof(objectState.lodModelOffsets[0])));
        for (unsigned int lod = 0; lod < lodCount; ++lod) {
            objectState.lodModelOffsets[lod] =
                readSaturnU16(lodTable + lod * 2u);
            if (objectState.lodModelOffsets[lod])
                objectState.lodCount =
                    static_cast<std::uint8_t>(lod + 1u);
        }

        const u16 modelTableOffset =
            objectState.lodModelOffsets[0];
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

        objectState.worldOrigin[0] =
            cellTranslation[0] + translation[0];
        objectState.worldOrigin[1] =
            cellTranslation[1] + translation[1];
        objectState.worldOrigin[2] =
            cellTranslation[2] + translation[2];

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

            const unsigned int lightingMode =
                (q.lightingControl >> 8) & 3u;
            ++out.lightingModes[lightingMode];

            record.lightingCount =
                static_cast<std::uint8_t>(
                    std::min<std::size_t>(4u, q.extra.size()));
            for (unsigned int i = 0;
                 i < record.lightingCount; ++i) {
                std::array<float,3> localNormal = {{
                    static_cast<float>(q.extra[i].normal[0]) / 4096.0f,
                    static_cast<float>(q.extra[i].normal[1]) / 4096.0f,
                    static_cast<float>(q.extra[i].normal[2]) / 4096.0f
                }};
                auto worldNormal = applyMat3(rotation, localNormal);
                const float nLen = std::sqrt(
                    worldNormal[0]*worldNormal[0] +
                    worldNormal[1]*worldNormal[1] +
                    worldNormal[2]*worldNormal[2]);
                if (nLen > 0.000001f) {
                    worldNormal[0] /= nLen;
                    worldNormal[1] /= nLen;
                    worldNormal[2] /= nLen;
                }

                record.lighting[i].normal[0] =
                    static_cast<std::int16_t>(
                        std::lround(worldNormal[0] * 4096.0f));
                record.lighting[i].normal[1] =
                    static_cast<std::int16_t>(
                        std::lround(worldNormal[1] * 4096.0f));
                record.lighting[i].normal[2] =
                    static_cast<std::int16_t>(
                        std::lround(worldNormal[2] * 4096.0f));
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

                auto v = applyMat3(rotation, local);
                v[0] += translation[0] + cellTranslation[0];
                v[1] += translation[1] + cellTranslation[1];
                v[2] += translation[2] + cellTranslation[2];

                DebugColorVertex dv{};
                dv.x = v[0]; dv.y = v[1]; dv.z = v[2];
                dv.r = r; dv.g = g; dv.b = b; dv.a = 255;

                out.vertices.push_back(dv);
                out.lightingVertices.push_back(dv);
            }
        }

        objectState.polygonCount =
            static_cast<std::uint32_t>(
                out.polygonRecords.size() -
                objectState.firstPolygon);
        out.objectStates.push_back(objectState);

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

    out.worldVertices = out.vertices;
    out.worldLightingVertices = out.lightingVertices;
    traceInitialRuinScriptNativeCalls(overlay);
    recoverInitialRuinCamera(overlay, out);

    lagi::platform::logging::writef(
        "[RoomDebug] world bounds min=(%.5f,%.5f,%.5f) max=(%.5f,%.5f,%.5f)\n",
        minV[0], minV[1], minV[2],
        maxV[0], maxV[1], maxV[2]);

    if (out.cameraValid) {
        float forward[3] = {
            out.cameraTarget[0] - out.cameraPosition[0],
            out.cameraTarget[1] - out.cameraPosition[1],
            out.cameraTarget[2] - out.cameraPosition[2]
        };
        const float lenSq =
            forward[0]*forward[0] +
            forward[1]*forward[1] +
            forward[2]*forward[2];
        if (lenSq > 0.0000001f) {
            const float inv = 1.0f / std::sqrt(lenSq);
            forward[0] *= inv;
            forward[1] *= inv;
            forward[2] *= inv;
        }

        float right[3] = {
            forward[2],
            0.0f,
            -forward[0]
        };
        const float rightLenSq =
            right[0]*right[0] +
            right[2]*right[2];
        if (rightLenSq > 0.0000001f) {
            const float inv = 1.0f / std::sqrt(rightLenSq);
            right[0] *= inv;
            right[2] *= inv;
        } else {
            right[0] = 1.0f;
            right[1] = 0.0f;
            right[2] = 0.0f;
        }

        float up[3] = {
            -forward[1] * right[2],
            forward[2] * right[0] -
                forward[0] * right[2],
            forward[1] * right[0]
        };
        const float upLenSq =
            up[0]*up[0] +
            up[1]*up[1] +
            up[2]*up[2];
        if (upLenSq > 0.0000001f) {
            const float inv = 1.0f / std::sqrt(upLenSq);
            up[0] *= inv;
            up[1] *= inv;
            up[2] *= inv;
        }

        constexpr float kPi =
            3.14159265358979323846f;
        const float cotHalf =
            1.0f /
            std::tan(
                (out.cameraFovDegrees * 0.5f) *
                kPi / 180.0f);
        float xScale =
            cotHalf * (352.0f / 320.0f);
        const float yScale =
            cotHalf *
            (176.0f / 112.0f) *
            (224.0f / 240.0f);
        xScale *=
            (4.0f / 3.0f) /
            (960.0f / 544.0f);

        float minDepth = 1.0e30f;
        float maxDepth = -1.0e30f;
        unsigned int inFront = 0;
        unsigned int inDepth = 0;
        unsigned int inHorizontal = 0;
        unsigned int inVertical = 0;
        unsigned int inFrustum = 0;

        for (const auto& v : out.worldVertices) {
            const float dx = v.x - out.cameraPosition[0];
            const float dy = v.y - out.cameraPosition[1];
            const float dz = v.z - out.cameraPosition[2];

            const float depth =
                dx*forward[0] +
                dy*forward[1] +
                dz*forward[2];
            const float viewX =
                dx*right[0] +
                dy*right[1] +
                dz*right[2];
            const float viewY =
                dx*up[0] +
                dy*up[1] +
                dz*up[2];

            minDepth = std::min(minDepth, depth);
            maxDepth = std::max(maxDepth, depth);

            const bool front = depth > 0.0f;
            const bool depthOk =
                depth >= out.cameraNear &&
                depth <= out.cameraFar;
            const bool horizontalOk =
                front &&
                std::fabs(viewX * xScale) <= depth;
            const bool verticalOk =
                front &&
                std::fabs(viewY * yScale) <= depth;

            if (front) ++inFront;
            if (depthOk) ++inDepth;
            if (horizontalOk) ++inHorizontal;
            if (verticalOk) ++inVertical;
            if (depthOk && horizontalOk && verticalOk)
                ++inFrustum;
        }

        lagi::platform::logging::writef(
            "[RoomDebug] camera depth min=%.5f max=%.5f front=%u/%u depth=%u/%u horiz=%u/%u vert=%u/%u frustum=%u/%u near=%.5f far=%.5f\n",
            minDepth,
            maxDepth,
            inFront,
            static_cast<unsigned>(out.worldVertices.size()),
            inDepth,
            static_cast<unsigned>(out.worldVertices.size()),
            inHorizontal,
            static_cast<unsigned>(out.worldVertices.size()),
            inVertical,
            static_cast<unsigned>(out.worldVertices.size()),
            inFrustum,
            static_cast<unsigned>(out.worldVertices.size()),
            out.cameraNear,
            out.cameraFar);
    }

    // Keep the renderer-facing room mesh in original Azel game space.
    // Diagnostic framing is now handled entirely by the debug camera.
    out.vertices = out.worldVertices;
    out.lightingVertices = out.worldLightingVertices;

    findInitialRuinSceneLight(overlay, out);

    std::vector<u8> cgb;
    if (lagi::disc::read_file("RUINMP.CGB", cgb) && !cgb.empty()) {
        constexpr u32 kRuinPaletteEA = 0x0605EBF8u;
        const u8* ruinPalette = nullptr;
        std::size_t ruinPaletteBytes = 0;

        if (kRuinPaletteEA >= overlay->m_base) {
            const u32 paletteOffset =
                kRuinPaletteEA - overlay->m_base;
            if (paletteOffset <= overlay->m_dataSize &&
                0x200u <= overlay->m_dataSize - paletteOffset) {
                ruinPalette =
                    overlay->m_data + paletteOffset;
                ruinPaletteBytes = 0x200u;
            }
        }

        decodeRoomTextures(
            cgb,
            ruinPalette,
            ruinPaletteBytes,
            out);
    }

    unsigned colorModes[8]{};
    for (const auto& record : out.polygonRecords)
        ++colorModes[record.colorMode() & 7u];

    lagi::platform::logging::writef(
        "[RoomDebug] camera=%s worldVerts=%u\n",
        out.cameraValid ? "resolved" : "missing",
        static_cast<unsigned>(out.worldVertices.size()));

    lagi::platform::logging::writef(
        "[RoomDebug] lighting modes 0/1/2/3: %u/%u/%u/%u sceneLight=%s\n",
        out.lightingModes[0], out.lightingModes[1],
        out.lightingModes[2], out.lightingModes[3],
        out.lightingValid ? "resolved" : "missing");

    lagi::platform::logging::writef(
        "[RoomDebug] texture modes 0/1/2/3/4/5/6/7: %u/%u/%u/%u/%u/%u/%u/%u\n",
        colorModes[0], colorModes[1], colorModes[2], colorModes[3],
        colorModes[4], colorModes[5], colorModes[6], colorModes[7]);
    for (std::size_t i = 0; i < out.polygonRecords.size(); ++i) {
        const auto& record = out.polygonRecords[i];
        if (record.colorMode() == 0) {
            lagi::platform::logging::writef(
                "[RoomDebug] mode0 poly=%u obj=%u localPoly=%u CTRL=%04X PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X\n",
                static_cast<unsigned>(i),
                record.model,
                record.polygonInModel,
                record.cmdCtrl,
                record.cmdPmod,
                record.cmdColr,
                record.cmdSrca,
                record.cmdSize);
        }
    }

    lagi::platform::logging::writef(
        "[RoomDebug] textures %u/%u, indirect CRAM pixels=%u, %s\n",
        out.decodedTextures,
        out.uniqueTextures,
        out.indirectCramPixels,
        out.texturesFullyResolved
            ? "fully resolved"
            : (out.texturesValid ? "CRAM required" : "fallback to polygon color"));

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
