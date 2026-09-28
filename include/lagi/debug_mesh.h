#pragma once
#include <cstdint>
#include <vector>

namespace lagi::azel {

struct DebugColorVertex {
    float x, y, z;
    std::uint8_t r, g, b, a;
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
};

struct BasicWingDebugMesh {
    std::vector<DebugColorVertex> vertices;
    std::vector<SaturnPolygonRecord> polygonRecords;
    unsigned int models = 0;
    unsigned int polygons = 0;

    // DRAGON0.CGB is loaded by the original game at VDP1 byte offset
    // 0x12000. The model bundle is relocated by 0x2400 VDP1 address units,
    // and 0x2400 << 3 == 0x12000.
    unsigned int cgbBytes = 0;
    unsigned int maxTextureEnd = 0;
    unsigned int maxLutEnd = 0;
    bool cgbReferencesValid = false;
};

bool build_basic_wing_debug_mesh(BasicWingDebugMesh& out);

} // namespace lagi::azel
