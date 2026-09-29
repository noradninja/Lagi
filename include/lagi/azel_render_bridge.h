#pragma once

#include <cstdint>

struct sProcessed3dModel;

namespace lagi::azel_bridge {

struct LiveVdp1Model;

struct SubmissionState {
    std::int32_t modelMatrix[12]{};
    std::int32_t lightVector[3]{};
    std::uint16_t lightColor[3]{};
    bool hasModelMatrix = false;
    bool hasLight = false;
    bool billboard = false;
};

// Reset per-frame submission diagnostics before Azel task draw execution.
void begin_frame();

// Number of model submissions observed through Azel's native render boundary
// during the current frame.
std::uint32_t submission_count();

// Most recently submitted Azel processed model. This remains opaque at the
// bridge layer until the live-model adapter converts it to Vdp1ModelSource.
sProcessed3dModel* last_model();

// CPU-side VDP1 model produced from the last live Azel submission.
// Null until a submitted sProcessed3dModel has been adapted successfully.
const LiveVdp1Model* last_adapted_model();

// Transform/light state captured at the same addObjectToDrawList boundary.
const SubmissionState& last_submission_state();

} // namespace lagi::azel_bridge
