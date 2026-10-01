#pragma once

#include <cstdint>
#include <vector>

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

struct RenderSubmission {
    sProcessed3dModel* model = nullptr;
    std::int32_t adaptedModelIndex = -1;
    std::int8_t bundleIndex = -1;
    std::uint32_t cellIndex = 0;
    std::uint32_t objectIndex = 0;
    std::uint32_t modelTableOffset = 0;
    SubmissionState state{};
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

// Ordered snapshot of every model submitted by Azel during the current task
// frame. Entries remain valid until the next begin_frame().
const std::vector<RenderSubmission>& submissions();
const LiveVdp1Model* adapted_model(std::uint32_t index);
void set_town_submission_context(
    std::int8_t bundleIndex,
    std::uint32_t cellIndex,
    std::uint32_t objectIndex,
    std::uint32_t modelTableOffset,
    const SubmissionState& state);

} // namespace lagi::azel_bridge
