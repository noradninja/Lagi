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
    std::uint32_t lightFalloff[3]{};
    bool hasModelMatrix = false;
    bool hasLight = false;
    bool billboard = false;

    // Task-owned objects can move independently of the cell/static cache.
    // The backend still consumes the same normal Azel model submission.
    bool dynamic = false;
};

struct Vdp1UiCommand {
    std::uint16_t cmdCtrl = 0;
    std::uint16_t cmdPmod = 0;
    std::uint16_t cmdColr = 0;
    std::uint16_t cmdSrca = 0;
    std::uint16_t cmdSize = 0;
    std::int16_t xa = 0, ya = 0;
    std::int16_t xb = 0, yb = 0;
    std::int16_t xc = 0, yc = 0;
    std::int16_t xd = 0, yd = 0;
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
void begin_frame(bool forceDynamicSubmissions = false);

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

// Ordered submissions being produced by the current Azel task frame.
const std::vector<RenderSubmission>& submissions();
const LiveVdp1Model* adapted_model(std::uint32_t index);

// Publish the completed task frame for renderer consumption. The published
// vectors are not modified by the next begin_frame()/task pass, which gives
// the renderer a stable frame boundary for the later threaded handoff.
void publish_frame();
void record_vdp1_ui_command(const Vdp1UiCommand& command);
const std::vector<Vdp1UiCommand>& published_vdp1_ui_commands();

const std::vector<RenderSubmission>& published_submissions();
const LiveVdp1Model* published_adapted_model(std::uint32_t index);
std::uint64_t published_frame_number();

// Number of previously unseen native model identities adapted while building
// the published Azel frame. Used only for field-streaming diagnostics.
std::uint32_t published_model_cache_misses();

// Diagnostic counts for explicitly supplied non-dynamic submission contexts.
// "Set" proves the generated draw hook ran; "consumed" proves the following
// native model submission received that context at the bridge boundary.
std::uint32_t published_explicit_static_contexts_set();
std::uint32_t published_explicit_static_contexts_consumed();

// Copy Azel's current lighting payload into an explicitly constructed
// submission state without also inheriting pCurrentMatrix (which may already
// contain the camera/view transform).
void capture_current_light(SubmissionState& state);

// Begin/end a draw scope where Azel's pCurrentMatrix is already view-space.
// Submissions inside the scope are converted back to world space before they
// are published to Neptune.
void begin_view_relative_submission_scope();
void end_view_relative_submission_scope();

// Field mode publishes Azel's native view matrix explicitly. The camera task
// captures it after applyCameraStatusToEngine(), then normal mode-3 model
// submissions can remove that view transform before reaching Neptune.
void capture_native_scene_view_matrix();
bool native_scene_view_matrix(std::int32_t out[12]);

void set_town_submission_context(
    std::int8_t bundleIndex,
    std::uint32_t cellIndex,
    std::uint32_t objectIndex,
    std::uint32_t modelTableOffset,
    const SubmissionState& state);

} // namespace lagi::azel_bridge
