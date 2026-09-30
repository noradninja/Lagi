#pragma once
#include <cstdint>
#include <vector>

namespace lagi::azel {

struct DebugColorVertex {
    float x, y, z;
    std::uint8_t r, g, b, a;
};

struct DebugTextureVertex {
    float x, y, z;
    float u, v;
};

struct DebugTexturedLitVertex {
    float x, y, z;
    float u, v;
    float light;
};

struct DecodedMode1Texture {
    std::uint16_t cmdPmod = 0;
    std::uint16_t cmdColr = 0;
    std::uint16_t cmdSrca = 0;
    std::uint16_t cmdSize = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector<std::uint32_t> rgba;
};

struct SaturnLightingExtra {
    std::int16_t normal[3]{};
    std::uint16_t color[3]{};
    bool hasColor = false;
};

struct SaturnGouraud555Quad {
    // Signed additive RGB offsets after Azel's 5-bit Gouraud quantization:
    //   ((gouraud5 - 16) / 31), one RGB triplet per original Saturn corner.
    float corner[4][3]{};
};

struct SaturnLightingNormalQuad {
    // Normalized after applying the reconstructed Basic Wing hierarchy pose.
    // These remain in assembled model space so the viewer can transform them
    // into camera space every frame.
    float corner[4][3]{};
};

struct BasicWingAnimationFrame {
    std::vector<DebugColorVertex> vertices;
    std::vector<SaturnLightingNormalQuad> lightingNormals;
};

struct SaturnPolygonRecord {
    std::uint16_t indices[4]{};
    std::uint16_t lightingControl = 0;
    std::uint16_t cmdCtrl = 0;
    std::uint16_t cmdPmod = 0;
    std::uint16_t cmdColr = 0;
    std::uint16_t cmdSrca = 0;
    std::uint16_t cmdSize = 0;
    unsigned int model = 0;
    unsigned int polygonInModel = 0;

    unsigned int textureWidth() const
    {
        return (cmdSize & 0x3F00u) >> 5;
    }

    unsigned int textureHeight() const
    {
        return cmdSize & 0x00FFu;
    }

    unsigned int colorMode() const
    {
        return (cmdPmod >> 3) & 0x7u;
    }

    unsigned int textureFlip() const
    {
        return (cmdCtrl >> 4) & 0x3u;
    }

    unsigned int textureByteAddress() const
    {
        return static_cast<unsigned int>(cmdSrca) << 3;
    }

    unsigned int lightingMode() const
    {
        return (lightingControl >> 8) & 0x3u;
    }

    SaturnLightingExtra lighting[4]{};
    std::uint8_t lightingCount = 0;
};

struct StaticRoomDebugMesh {
    // Normalized diagnostic vertices used by viewer modes 5/6.
    std::vector<DebugColorVertex> vertices;
    std::vector<DebugColorVertex> lightingVertices;

    // Untouched assembled town-space geometry for the authentic camera mode.
    std::vector<DebugColorVertex> worldVertices;
    std::vector<DebugColorVertex> worldLightingVertices;

    std::vector<SaturnPolygonRecord> polygonRecords;
    std::vector<SaturnGouraud555Quad> gouraud555;

    std::vector<DecodedMode1Texture> decodedTextureData;
    std::vector<std::uint16_t> polygonTextureIndices;

    unsigned int objects = 0;
    unsigned int models = 0;
    unsigned int polygons = 0;
    unsigned int uniqueTextures = 0;
    unsigned int decodedTextures = 0;
    unsigned int indirectCramPixels = 0;

    // First-scene lighting recovered from the town script's
    // townCamera_setup() call. Direction is normalized world-space light
    // input before setupLight() negates/scales it.
    float lightDirection[3]{};
    std::uint8_t lightColor[3]{};
    std::uint32_t lightFalloff[3]{};
    unsigned int lightingModes[4]{};
    bool lightingValid = false;

    float cameraPosition[3]{};
    float cameraTarget[3]{};
    float cameraUp[3]{};
    float cameraFovDegrees = 80.0f;
    float cameraNear = 0x800 / 65536.0f;
    float cameraFar = 0xF000 / 65536.0f;
    bool cameraValid = false;

    bool texturesValid = false;
    bool texturesFullyResolved = false;
    bool truncated = false;
};

struct BasicWingDebugMesh {
    std::vector<DebugColorVertex> vertices;
    std::vector<DebugColorVertex> lightingVertices;
    std::vector<SaturnPolygonRecord> polygonRecords;
    std::vector<SaturnGouraud555Quad> gouraud555;
    std::vector<SaturnLightingNormalQuad> lightingNormals;

    // Morph-screen Basic Wing animation (dragonAnimOffsets[0] = 0x10C).
    std::vector<BasicWingAnimationFrame> animationFrames;
    std::uint16_t animationFlags = 0;
    std::uint16_t animationFrameCount = 0;
    bool animationValid = false;

    unsigned int models = 0;
    unsigned int polygons = 0;

    // DRAGON0.CGB is loaded by the original game at VDP1 byte offset
    // 0x12000. The model bundle is relocated by 0x2400 VDP1 address units,
    // and 0x2400 << 3 == 0x12000.
    unsigned int cgbBytes = 0;
    unsigned int maxTextureEnd = 0;
    unsigned int maxLutEnd = 0;
    bool cgbReferencesValid = false;

    // First-pass VDP1 mode-1 texture decoder diagnostics.
    unsigned int uniqueTextures = 0;
    unsigned int decodedTextures = 0;
    unsigned int decodedPixels = 0;
    unsigned int transparentPixels = 0;
    unsigned int endCodePixels = 0;
    unsigned int directRgb555Pixels = 0;
    unsigned int indirectCramPixels = 0;
    bool mode1DecodeValid = false;
    bool mode1DecodeFullyResolved = false;

    // Raw DRAGON0.MCB lighting/Gouraud payload diagnostics.
    unsigned int lightingModeCounts[4]{};
    unsigned int lightingExtraRecords = 0;
    unsigned int lightingColoredRecords = 0;
    bool lightingPayloadValid = false;

    // Retained by the decoder for the native GXM texture upload path.
    std::vector<DecodedMode1Texture> decodedTextureData;
    std::vector<std::uint16_t> polygonTextureIndices;
};

bool build_basic_wing_debug_mesh(BasicWingDebugMesh& out);
bool build_first_ruin_room_debug_mesh(StaticRoomDebugMesh& out);

} // namespace lagi::azel
