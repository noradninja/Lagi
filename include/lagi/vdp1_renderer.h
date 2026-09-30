#pragma once

#include "lagi/debug_mesh.h"

#include <cstddef>
#include <cstdint>

namespace lagi::platform::renderer {

// Rendering modes supported by the hardware-proven native VDP1 path.
// The first three preserve the Saturn texture/Gouraud behavior; the final
// two remain useful diagnostics for the standalone regression viewer.
enum class Vdp1RenderMode : std::uint8_t {
    Textured = 0,
    TexturedGouraud = 1,
    GouraudGrayscale = 2,
    PolygonColor = 3,
    Wireframe = 4,
    TexturedGouraudNewton = 5,
    TexturedPayloadProbe = 6,
    TexturedGouraudNoInverse = 7,
    TexturedGouraudNoQuant = 8,
    TexturedRgb555NoGouraud = 9,
    TexturedGouraudFinalQuant = 10,
};

// Non-owning CPU-side description of one triangulated Saturn VDP1 model.
//
// Each original Saturn quad is represented by six vertices/indices
// (0,1,2, 0,2,3). Polygon metadata, texture selection, and four-corner
// Gouraud values remain one entry per original quad so the backend can
// reconstruct quad semantics instead of treating the two triangles as
// unrelated primitives.
//
// This deliberately contains no Basic Wing-specific state. Any Azel model
// adapter that can provide this data layout can use the same native Vita
// submission path.
struct Vdp1ModelSource {
    const azel::DebugColorVertex* vertices = nullptr;
    const azel::DebugColorVertex* lightingVertices = nullptr;
    std::size_t vertexCount = 0;

    const azel::SaturnPolygonRecord* polygons = nullptr;
    const azel::SaturnGouraud555Quad* gouraud555 = nullptr;
    std::size_t polygonCount = 0;

    const azel::DecodedMode1Texture* textures = nullptr;
    std::size_t textureCount = 0;

    const std::uint16_t* polygonTextureIndices = nullptr;
    std::size_t polygonTextureIndexCount = 0;

    bool geometryValid() const
    {
        return vertices &&
               lightingVertices &&
               vertexCount != 0 &&
               polygons &&
               gouraud555 &&
               polygonCount != 0 &&
               vertexCount == polygonCount * 6u;
    }

    bool texturesValid() const
    {
        return textures &&
               textureCount != 0 &&
               polygonTextureIndices &&
               polygonTextureIndexCount == polygonCount;
    }

    bool valid() const
    {
        return geometryValid();
    }
};

// Per-draw state supplied by the caller. The matrix uses the same row-vector
// convention as texture_v.cg: mul(float4(position, 1), wvp).
struct Vdp1DrawState {
    float wvp[16]{};
    Vdp1RenderMode mode = Vdp1RenderMode::Textured;
};

// Upload one model into the current native Vita VDP1 resource slot. M1 keeps
// a single resident model because the regression viewer only needs one; the
// interface is deliberately model-agnostic so the next integration step can
// feed live Azel model data without inheriting Basic Wing viewer state.
bool prepare_vdp1_model(const Vdp1ModelSource& model);

// Submit the prepared model into the currently active GXM scene using the
// supplied transform and render mode.
bool submit_vdp1_model(
    const Vdp1ModelSource& model,
    const Vdp1DrawState& drawState);

} // namespace lagi::platform::renderer
