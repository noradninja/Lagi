#include "lagi/lagi_compat.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_live_model_adapter.h"
#include "lagi/platform.h"

#include <cstdint>
#include <cstring>
#include <unordered_map>

struct sProcessed3dModel;

// Minimal layout mirrors for the live Azel globals we capture. These match the
// portable prefix used by upstream Azel but avoid including common.h, which
// also depends on task/VDP2 types not present in Lagi's stripped Vita build.
struct LagiMatrix4x3
{
    fixedPoint m[3][4];
};

struct LagiCurrentLightVector
{
    fixedPoint lightVector[3];
    u16 color[3];
};

// Full Azel builds provide these globals. They are weak here so the current
// smoke runtime can link before 3dEngine.cpp/menu_dragonMorph.cpp are part of
// the Vita executable.
extern LagiMatrix4x3* pCurrentMatrix __attribute__((weak));
extern LagiCurrentLightVector currentLightVector_M __attribute__((weak));

namespace lagi::azel_bridge {

static std::uint32_t g_submissionCount = 0;
static sProcessed3dModel* g_lastModel = nullptr;
static const LiveVdp1Model* g_lastAdaptedModel = nullptr;
static SubmissionState g_lastState{};
static std::unordered_map<sProcessed3dModel*, LiveVdp1Model> g_modelCache;
static std::vector<const LiveVdp1Model*> g_adaptedModels;
static std::vector<RenderSubmission> g_submissions;
static std::vector<const LiveVdp1Model*> g_publishedAdaptedModels;
static std::vector<RenderSubmission> g_publishedSubmissions;
static std::uint64_t g_publishedFrameNumber = 0;
static RenderSubmission g_pendingTownSubmission{};
static bool g_hasPendingTownSubmission = false;
static bool g_reportedFirstSubmission = false;
static bool g_reportedFirstAdaptedModel = false;
static unsigned int g_reportedCommonSubmissions = 0;
static unsigned int g_reportedRuinSubmissions = 0;

void begin_frame()
{
    g_submissionCount = 0;
    g_lastModel = nullptr;
    g_lastAdaptedModel = nullptr;
    g_lastState = {};
    g_adaptedModels.clear();
    g_submissions.clear();
    g_pendingTownSubmission = {};
    g_hasPendingTownSubmission = false;
}

std::uint32_t submission_count()
{
    return g_submissionCount;
}

sProcessed3dModel* last_model()
{
    return g_lastModel;
}

const LiveVdp1Model* last_adapted_model()
{
    return g_lastAdaptedModel;
}

const SubmissionState& last_submission_state()
{
    return g_lastState;
}

const std::vector<RenderSubmission>& submissions()
{
    return g_submissions;
}

const LiveVdp1Model* adapted_model(std::uint32_t index)
{
    return index < g_adaptedModels.size()
        ? g_adaptedModels[index]
        : nullptr;
}

void publish_frame()
{
    // LiveVdp1Model objects themselves live in g_modelCache and therefore
    // remain stable across frames. Only the per-frame ordering/state vectors
    // need to be snapshotted here.
    g_publishedSubmissions = g_submissions;
    g_publishedAdaptedModels = g_adaptedModels;
    ++g_publishedFrameNumber;
}

const std::vector<RenderSubmission>& published_submissions()
{
    return g_publishedSubmissions;
}

const LiveVdp1Model* published_adapted_model(std::uint32_t index)
{
    return index < g_publishedAdaptedModels.size()
        ? g_publishedAdaptedModels[index]
        : nullptr;
}

std::uint64_t published_frame_number()
{
    return g_publishedFrameNumber;
}

void set_town_submission_context(
    std::int8_t bundleIndex,
    std::uint32_t cellIndex,
    std::uint32_t objectIndex,
    std::uint32_t modelTableOffset,
    const SubmissionState& state)
{
    g_pendingTownSubmission = {};
    g_pendingTownSubmission.bundleIndex = bundleIndex;
    g_pendingTownSubmission.cellIndex = cellIndex;
    g_pendingTownSubmission.objectIndex = objectIndex;
    g_pendingTownSubmission.modelTableOffset = modelTableOffset;
    g_pendingTownSubmission.state = state;
    g_hasPendingTownSubmission = true;
}

static void capture_runtime_state(bool billboard)
{
    g_lastState = {};
    g_lastState.billboard = billboard;

    // For normal objects Azel's pCurrentMatrix already contains camera/view
    // and model transforms at submission time. Billboard capture will later
    // substitute cameraProperties2.m88_billboardViewMatrix when that path is
    // linked into the Vita runtime.
    if (&pCurrentMatrix && pCurrentMatrix) {
        for (unsigned int row = 0; row < 3; ++row) {
            for (unsigned int col = 0; col < 4; ++col) {
                g_lastState.modelMatrix[row * 4u + col] =
                    pCurrentMatrix->m[row][col].asS32();
            }
        }
        g_lastState.hasModelMatrix = true;
    }

    if (&currentLightVector_M) {
        for (unsigned int i = 0; i < 3; ++i) {
            g_lastState.lightVector[i] =
                currentLightVector_M.lightVector[i].asS32();
            g_lastState.lightColor[i] =
                currentLightVector_M.color[i];
        }
        g_lastState.hasLight = true;
    }
}

static void record_submission(sProcessed3dModel* model, bool billboard)
{
    if (!model)
        return;

    ++g_submissionCount;
    g_lastModel = model;
    if (g_hasPendingTownSubmission) {
        g_lastState = g_pendingTownSubmission.state;
        g_lastState.billboard = billboard;
    } else {
        capture_runtime_state(billboard);
    }

    std::int32_t adaptedIndex = -1;
    auto cached = g_modelCache.find(model);
    if (cached == g_modelCache.end()) {
        LiveVdp1Model adapted{};
        if (adapt_processed_model(model, adapted))
            cached = g_modelCache.emplace(model, std::move(adapted)).first;
    }
    if (cached != g_modelCache.end()) {
        g_lastAdaptedModel = &cached->second;
        g_adaptedModels.push_back(g_lastAdaptedModel);
        adaptedIndex = static_cast<std::int32_t>(g_adaptedModels.size() - 1u);

        // Hardware bring-up diagnostic: identify the native submission's VDP1
        // bundle from its relocated texture address and print the exact Azel
        // matrix that arrived at addObjectToDrawList(). This deliberately does
        // not alter transforms or runtime ownership.
        if (!g_lastAdaptedModel->polygons.empty() &&
            g_lastState.hasModelMatrix) {
            const auto& first = g_lastAdaptedModel->polygons.front();
            const std::uint16_t srca = first.cmdSrca;
            const bool common =
                srca >= 0xEE08u && srca < 0xF800u;
            const bool ruin =
                srca >= 0x77E4u && srca < 0xEE08u;
            unsigned int* counter =
                common ? &g_reportedCommonSubmissions :
                (ruin ? &g_reportedRuinSubmissions : nullptr);
            if (counter && *counter < 8u) {
                lagi::platform::logging::writef(
                    "[TownSubmit] %s #%u model=%p polys=%u "
                    "SRCA=%04X bill=%u "
                    "T=(%.5f,%.5f,%.5f) "
                    "R0=(%.4f,%.4f,%.4f) "
                    "R1=(%.4f,%.4f,%.4f) "
                    "R2=(%.4f,%.4f,%.4f)\n",
                    common ? "COMMON" : "RUIN",
                    *counter,
                    model,
                    static_cast<unsigned>(
                        g_lastAdaptedModel->polygons.size()),
                    static_cast<unsigned>(srca),
                    billboard ? 1u : 0u,
                    g_lastState.modelMatrix[3] / 65536.0f,
                    g_lastState.modelMatrix[7] / 65536.0f,
                    g_lastState.modelMatrix[11] / 65536.0f,
                    g_lastState.modelMatrix[0] / 65536.0f,
                    g_lastState.modelMatrix[1] / 65536.0f,
                    g_lastState.modelMatrix[2] / 65536.0f,
                    g_lastState.modelMatrix[4] / 65536.0f,
                    g_lastState.modelMatrix[5] / 65536.0f,
                    g_lastState.modelMatrix[6] / 65536.0f,
                    g_lastState.modelMatrix[8] / 65536.0f,
                    g_lastState.modelMatrix[9] / 65536.0f,
                    g_lastState.modelMatrix[10] / 65536.0f);
                ++*counter;
            }
        }
    } else {
        g_lastAdaptedModel = nullptr;
    }
    RenderSubmission submission = g_hasPendingTownSubmission
        ? g_pendingTownSubmission : RenderSubmission{};
    submission.model = model;
    submission.adaptedModelIndex = adaptedIndex;
    submission.state = g_lastState;
    g_submissions.push_back(submission);
    g_pendingTownSubmission = {};
    g_hasPendingTownSubmission = false;

    if (!g_reportedFirstSubmission) {
        lagi::platform::renderer::status(
            billboard
                ? "[PASS] AZEL LIVE BILLBOARD SUBMISSION"
                : "[PASS] AZEL LIVE MODEL SUBMISSION",
            0xFF70E0A0u);
        g_reportedFirstSubmission = true;
    }

    if (g_lastAdaptedModel && !g_reportedFirstAdaptedModel) {
        lagi::platform::renderer::status(
            "[PASS] AZEL MODEL -> VDP1 SOURCE",
            0xFF70E0A0u);
        g_reportedFirstAdaptedModel = true;
    }
}

} // namespace lagi::azel_bridge

// These are the same render-boundary symbols used throughout Azel field,
// town, battle, dragon, and menu code. Desktop Azel implements them in
// 3dEngine_flush.cpp; that implementation is excluded by USE_NULL_RENDERER.
// Lagi owns the Vita implementation.
void addObjectToDrawList(sProcessed3dModel* model)
{
    lagi::azel_bridge::record_submission(model, false);
}

void addBillBoardToDrawList(sProcessed3dModel* model)
{
    lagi::azel_bridge::record_submission(model, true);
}

sProcessed3dModel* drawCurrentObjectInstanced(
    sProcessed3dModel* model,
    sProcessed3dModel*)
{
    lagi::azel_bridge::record_submission(model, false);
    return model;
}
