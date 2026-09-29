#pragma once

#include "lagi/vdp1_renderer.h"

#include <vector>

struct sProcessed3dModel;

namespace lagi::azel_bridge {

struct LiveVdp1Model {
    std::vector<lagi::azel::DebugColorVertex> vertices;
    std::vector<lagi::azel::DebugColorVertex> lightingVertices;
    std::vector<lagi::azel::SaturnPolygonRecord> polygons;
    std::vector<lagi::azel::SaturnGouraud555Quad> gouraud555;

    lagi::platform::renderer::Vdp1ModelSource source() const;
};

bool adapt_processed_model(
    sProcessed3dModel* model,
    LiveVdp1Model& out);

} // namespace lagi::azel_bridge
