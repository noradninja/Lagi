#pragma once
#include <cstdint>
#include <vector>

namespace lagi::azel {

struct DebugColorVertex {
    float x, y, z;
    std::uint8_t r, g, b, a;
};

struct BasicWingDebugMesh {
    std::vector<DebugColorVertex> vertices;
    unsigned int models = 0;
    unsigned int polygons = 0;
};

bool build_basic_wing_debug_mesh(BasicWingDebugMesh& out);

} // namespace lagi::azel
