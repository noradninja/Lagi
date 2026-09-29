#include "lagi/azel_compat.h"
#include "common.h"
#include "lagi/azel_live_model_adapter.h"

#include <array>
#include <cstdint>
#include <vector>

struct sProcessed3dModel;

// Vita does not include upstream processModel.h because its tail contains
// BGFX-only GPU resources. This prefix mirrors the portable CPU fields at the
// beginning of sProcessed3dModel exactly; only those fields are consumed here.
struct LagiProcessed3dModelPrefix
{
    struct QuadExtra {
        sVec3_S16_12_4 normals;
        sVec3_U16 colors;
    };

    struct Quad {
        std::array<u16, 4> indices;
        u16 lightingControl;
        u16 cmdCtrl;
        u16 cmdPmod;
        u16 cmdColr;
        u16 cmdSrca;
        u16 cmdSize;
        std::vector<QuadExtra> extraData;
    };

    u8* base;
    fixedPoint radius;
    u32 numVertices;
    std::vector<sVec3_S16_12_4> vertices;
    std::vector<Quad> quads;
};

namespace lagi::azel_bridge {

lagi::platform::renderer::Vdp1ModelSource
LiveVdp1Model::source() const
{
    lagi::platform::renderer::Vdp1ModelSource out{};
    out.vertices = vertices.data();
    out.lightingVertices = lightingVertices.data();
    out.vertexCount = vertices.size();
    out.polygons = polygons.data();
    out.gouraud555 = gouraud555.data();
    out.polygonCount = polygons.size();

    // Live texture memory is connected in the next stage. Geometry/debug
    // submission is valid without a texture set.
    out.textures = nullptr;
    out.textureCount = 0;
    out.polygonTextureIndices = nullptr;
    out.polygonTextureIndexCount = 0;
    return out;
}

bool adapt_processed_model(
    sProcessed3dModel* opaqueModel,
    LiveVdp1Model& out)
{
    out = {};

    if (!opaqueModel)
        return false;

    const auto* model =
        reinterpret_cast<const LagiProcessed3dModelPrefix*>(opaqueModel);

    if (model->vertices.empty() ||
        model->quads.empty() ||
        model->vertices.size() != model->numVertices)
        return false;

    const std::size_t polygonCount = model->quads.size();
    if (polygonCount > (65535u / 6u))
        return false;

    out.vertices.reserve(polygonCount * 6u);
    out.lightingVertices.reserve(polygonCount * 6u);
    out.polygons.reserve(polygonCount);
    out.gouraud555.resize(polygonCount);

    static constexpr unsigned int triCorners[6] = {
        0, 1, 2, 0, 2, 3
    };

    for (std::size_t p = 0; p < polygonCount; ++p) {
        const auto& q = model->quads[p];

        for (unsigned int corner = 0; corner < 4; ++corner) {
            if (q.indices[corner] >= model->vertices.size())
                return false;
        }

        lagi::azel::SaturnPolygonRecord record{};
        for (unsigned int corner = 0; corner < 4; ++corner)
            record.indices[corner] = q.indices[corner];

        record.lightingControl = q.lightingControl;
        record.cmdCtrl = q.cmdCtrl;
        record.cmdPmod = q.cmdPmod;
        record.cmdColr = q.cmdColr;
        record.cmdSrca = q.cmdSrca;
        record.cmdSize = q.cmdSize;
        record.model = 0;
        record.polygonInModel = static_cast<unsigned int>(p);

        const unsigned int lightingMode =
            (q.lightingControl >> 8) & 3u;
        const unsigned int normalCount =
            lightingMode == 1 ? 1u :
            ((lightingMode == 2 || lightingMode == 3) ? 4u : 0u);

        record.lightingCount = static_cast<std::uint8_t>(
            normalCount > 4u ? 4u : normalCount);

        for (unsigned int corner = 0;
             corner < record.lightingCount; ++corner) {
            const unsigned int sourceIndex =
                lightingMode == 1 ? 0u : corner;
            if (sourceIndex >= q.extraData.size())
                break;

            const auto& extra = q.extraData[sourceIndex];
            record.lighting[corner].normal[0] =
                static_cast<std::int16_t>(extra.normals[0]);
            record.lighting[corner].normal[1] =
                static_cast<std::int16_t>(extra.normals[1]);
            record.lighting[corner].normal[2] =
                static_cast<std::int16_t>(extra.normals[2]);

            if (lightingMode == 2) {
                record.lighting[corner].color[0] = extra.colors[0];
                record.lighting[corner].color[1] = extra.colors[1];
                record.lighting[corner].color[2] = extra.colors[2];
                record.lighting[corner].hasColor = true;
            }
        }

        out.polygons.push_back(record);

        // Give the geometry-only live path stable per-polygon debug colors.
        // This is not game shading; it exists only until live VDP1 textures
        // and Azel lighting are connected.
        const std::uint8_t debugR =
            static_cast<std::uint8_t>(64u + (p * 53u) % 160u);
        const std::uint8_t debugG =
            static_cast<std::uint8_t>(64u + (p * 97u) % 160u);
        const std::uint8_t debugB =
            static_cast<std::uint8_t>(64u + (p * 29u) % 160u);

        for (unsigned int k = 0; k < 6; ++k) {
            const unsigned int corner = triCorners[k];
            const auto& src = model->vertices[q.indices[corner]];
            const sVec3_FP fp = src.toSVec3_FP();

            lagi::azel::DebugColorVertex v{};
            v.x = fp[0].toFloat();
            v.y = fp[1].toFloat();
            v.z = fp[2].toFloat();
            v.r = debugR;
            v.g = debugG;
            v.b = debugB;
            v.a = 255;

            out.vertices.push_back(v);
            out.lightingVertices.push_back(v);
        }
    }

    return out.source().geometryValid();
}

} // namespace lagi::azel_bridge
