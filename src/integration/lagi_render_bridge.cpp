#include "lagi/lagi_compat.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_live_model_adapter.h"
#include "lagi/platform.h"

#include <algorithm>
#include <cstdlib>
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

struct LagiLightSetup
{
    fixedPoint direction[4];
    u32 falloff[3];
};

// Full Azel builds provide these globals. They are weak here so the current
// smoke runtime can link before 3dEngine.cpp/menu_dragonMorph.cpp are part of
// the Vita executable.
extern LagiMatrix4x3* pCurrentMatrix __attribute__((weak));
extern LagiCurrentLightVector currentLightVector_M __attribute__((weak));
extern LagiLightSetup lightSetup __attribute__((weak));

namespace lagi::azel_bridge {

static std::uint32_t g_submissionCount = 0;
static std::uint32_t g_modelCacheMisses = 0;
static std::uint32_t g_publishedModelCacheMisses = 0;
static std::uint32_t g_explicitStaticContextsSet = 0;
static std::uint32_t g_explicitStaticContextsConsumed = 0;
static std::uint32_t g_publishedExplicitStaticContextsSet = 0;
static std::uint32_t g_publishedExplicitStaticContextsConsumed = 0;
static sProcessed3dModel* g_lastModel = nullptr;
static const LiveVdp1Model* g_lastAdaptedModel = nullptr;
static SubmissionState g_lastState{};
static std::unordered_map<sProcessed3dModel*, LiveVdp1Model> g_modelCache;
static std::vector<const LiveVdp1Model*> g_adaptedModels;
static std::vector<RenderSubmission> g_submissions;
static std::vector<const LiveVdp1Model*> g_publishedAdaptedModels;
static std::vector<RenderSubmission> g_publishedSubmissions;
static std::vector<Vdp1UiCommand> g_vdp1UiCommands;
static std::vector<Vdp1UiCommand> g_publishedVdp1UiCommands;
static std::uint64_t g_publishedFrameNumber = 0;
static RenderSubmission g_pendingTownSubmission{};
static bool g_hasPendingTownSubmission = false;
static bool g_reportedFirstSubmission = false;
static bool g_reportedFirstAdaptedModel = false;
static bool g_viewRelativeScope = false;
static bool g_forceDynamicSubmissions = false;
static std::int32_t g_viewScopeMatrix[12]{};
static bool g_nativeSceneViewValid = false;
static std::int32_t g_nativeSceneViewMatrix[12]{};

// Bounded FLD_A3 diagnostic: compare the static-grid state synthesized by the
// generated hook with Azel's actual pCurrentMatrix/currentLightVector_M at the
// native addObjectToDrawList() boundary. This is observation only; it never
// changes submission state or rendering.
static unsigned int g_staticLightingCompareBudget = 96u;

static void copyMatrixRaw(const LagiMatrix4x3& source, std::int32_t out[12])
{
    for (unsigned int row = 0; row < 3; ++row)
        for (unsigned int col = 0; col < 4; ++col)
            out[row * 4u + col] = source.m[row][col].asS32();
}

static void removeViewTransform(
    const std::int32_t view[12],
    const LagiMatrix4x3& current,
    std::int32_t out[12])
{
    // Azel's town actor draw path builds pCurrentMatrix as view * world.
    // Town cameras are rigid transforms, so inverse(view) is R^T with
    // translation -R^T*t. Keep the calculation in 16.16 fixed point.
    std::int32_t inv[12]{};
    inv[0] = view[0]; inv[1] = view[4]; inv[2] = view[8];
    inv[4] = view[1]; inv[5] = view[5]; inv[6] = view[9];
    inv[8] = view[2]; inv[9] = view[6]; inv[10] = view[10];

    for (unsigned int row = 0; row < 3; ++row) {
        std::int64_t t = 0;
        t += static_cast<std::int64_t>(inv[row * 4u + 0]) * view[3];
        t += static_cast<std::int64_t>(inv[row * 4u + 1]) * view[7];
        t += static_cast<std::int64_t>(inv[row * 4u + 2]) * view[11];
        inv[row * 4u + 3] = static_cast<std::int32_t>(-(t >> 16));
    }

    std::int32_t cur[12]{};
    copyMatrixRaw(current, cur);
    for (unsigned int row = 0; row < 3; ++row) {
        for (unsigned int col = 0; col < 3; ++col) {
            std::int64_t v = 0;
            for (unsigned int k = 0; k < 3; ++k)
                v += static_cast<std::int64_t>(inv[row * 4u + k]) *
                     cur[k * 4u + col];
            out[row * 4u + col] = static_cast<std::int32_t>(v >> 16);
        }

        std::int64_t t = static_cast<std::int64_t>(
            inv[row * 4u + 3]) << 16;
        for (unsigned int k = 0; k < 3; ++k)
            t += static_cast<std::int64_t>(inv[row * 4u + k]) *
                 cur[k * 4u + 3];
        out[row * 4u + 3] = static_cast<std::int32_t>(t >> 16);
    }
}

void begin_frame(bool forceDynamicSubmissions)
{
    g_forceDynamicSubmissions = forceDynamicSubmissions;
    g_submissionCount = 0;
    g_modelCacheMisses = 0;
    g_explicitStaticContextsSet = 0;
    g_explicitStaticContextsConsumed = 0;
    g_lastModel = nullptr;
    g_lastAdaptedModel = nullptr;
    g_lastState = {};
    g_adaptedModels.clear();
    g_submissions.clear();
    g_vdp1UiCommands.clear();
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
    g_publishedVdp1UiCommands = g_vdp1UiCommands;
    g_publishedAdaptedModels = g_adaptedModels;
    g_publishedModelCacheMisses = g_modelCacheMisses;
    g_publishedExplicitStaticContextsSet = g_explicitStaticContextsSet;
    g_publishedExplicitStaticContextsConsumed =
        g_explicitStaticContextsConsumed;
    ++g_publishedFrameNumber;
}

void record_vdp1_ui_command(const Vdp1UiCommand& command)
{
    g_vdp1UiCommands.push_back(command);
}

const std::vector<Vdp1UiCommand>& published_vdp1_ui_commands()
{
    return g_publishedVdp1UiCommands;
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

std::uint32_t published_model_cache_misses()
{
    return g_publishedModelCacheMisses;
}

std::uint32_t published_explicit_static_contexts_set()
{
    return g_publishedExplicitStaticContextsSet;
}

std::uint32_t published_explicit_static_contexts_consumed()
{
    return g_publishedExplicitStaticContextsConsumed;
}

void capture_current_light(SubmissionState& state)
{
    if (&currentLightVector_M) {
        for (unsigned int i = 0; i < 3; ++i) {
            state.lightVector[i] =
                currentLightVector_M.lightVector[i].asS32();
            state.lightColor[i] =
                currentLightVector_M.color[i];
        }

        // In field mode currentLightVector_M is paired with view-relative
        // model matrices. When the bridge publishes world-space geometry,
        // rotate the light back through inverse(view) too. Later Neptune's
        // model-space transpose multiply then reproduces the same dot product.
        if (g_forceDynamicSubmissions && g_nativeSceneViewValid) {
            const std::int32_t in[3] = {
                state.lightVector[0],
                state.lightVector[1],
                state.lightVector[2]};
            for (unsigned int axis = 0; axis < 3; ++axis) {
                std::int64_t value = 0;
                value += static_cast<std::int64_t>(
                    g_nativeSceneViewMatrix[axis]) * in[0];
                value += static_cast<std::int64_t>(
                    g_nativeSceneViewMatrix[4u + axis]) * in[1];
                value += static_cast<std::int64_t>(
                    g_nativeSceneViewMatrix[8u + axis]) * in[2];
                state.lightVector[axis] =
                    static_cast<std::int32_t>(value >> 16);
            }
        }

        if (&lightSetup) {
            state.lightFalloff[0] = lightSetup.falloff[0];
            state.lightFalloff[1] = lightSetup.falloff[1];
            state.lightFalloff[2] = lightSetup.falloff[2];
        }
        state.hasLight = true;
    }
}

void begin_view_relative_submission_scope()
{
    g_viewRelativeScope = false;
    if (&pCurrentMatrix && pCurrentMatrix) {
        copyMatrixRaw(*pCurrentMatrix, g_viewScopeMatrix);
        g_viewRelativeScope = true;
    }
}

void end_view_relative_submission_scope()
{
    g_viewRelativeScope = false;
}

void capture_native_scene_view_matrix()
{
    if (&pCurrentMatrix && pCurrentMatrix) {
        copyMatrixRaw(*pCurrentMatrix, g_nativeSceneViewMatrix);
        g_nativeSceneViewValid = true;
    }
}

bool native_scene_view_matrix(std::int32_t out[12])
{
    if (!out || !g_nativeSceneViewValid)
        return false;
    std::memcpy(out, g_nativeSceneViewMatrix, sizeof(g_nativeSceneViewMatrix));
    return true;
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
    if (!state.dynamic)
        ++g_explicitStaticContextsSet;
}

static void capture_runtime_state(bool billboard)
{
    g_lastState = {};
    g_lastState.billboard = billboard;
    // Field mode still defaults task-owned submissions to dynamic, but the
    // native field view is now removed below so every submission presented by
    // Neptune uses world-space geometry. Explicit environment-grid contexts
    // can therefore opt into the static cache without affecting actors,
    // effects, moving machinery, or billboards.
    g_lastState.dynamic =
        g_forceDynamicSubmissions || g_viewRelativeScope || billboard;

    // Azel's pCurrentMatrix contains camera/view and model transforms at the
    // normal submission boundary. Mode 3 strips the captured native view;
    // town view-relative scopes use their existing scoped view snapshot.
    if (&pCurrentMatrix && pCurrentMatrix) {
        if (g_viewRelativeScope) {
            removeViewTransform(
                g_viewScopeMatrix,
                *pCurrentMatrix,
                g_lastState.modelMatrix);
        } else if (g_forceDynamicSubmissions && g_nativeSceneViewValid) {
            // Field submissions are authored under Azel's current view matrix.
            // Remove only that native camera transform so Neptune receives a
            // stable world-space model transform and can apply the field
            // camera exactly once at presentation.
            removeViewTransform(
                g_nativeSceneViewMatrix,
                *pCurrentMatrix,
                g_lastState.modelMatrix);
        } else {
            copyMatrixRaw(*pCurrentMatrix, g_lastState.modelMatrix);
        }
        g_lastState.hasModelMatrix = true;
    }

    capture_current_light(g_lastState);
}

static void record_submission(sProcessed3dModel* model, bool billboard)
{
    if (!model)
        return;

    ++g_submissionCount;
    g_lastModel = model;
    if (g_hasPendingTownSubmission &&
        !g_pendingTownSubmission.state.dynamic &&
        !billboard)
        ++g_explicitStaticContextsConsumed;
    if (g_hasPendingTownSubmission) {
        // For rigid FLD_A3 environment submissions, validate that the
        // generated static hook's world-space matrix/light pair is equivalent
        // to the native Azel view-space pair at this exact draw boundary.
        if (g_staticLightingCompareBudget != 0u &&
            !g_pendingTownSubmission.state.dynamic &&
            !billboard &&
            g_forceDynamicSubmissions &&
            g_nativeSceneViewValid &&
            &pCurrentMatrix && pCurrentMatrix &&
            &currentLightVector_M) {
            std::int32_t actualWorld[12]{};
            removeViewTransform(
                g_nativeSceneViewMatrix, *pCurrentMatrix, actualWorld);

            std::int32_t nativeModelLight[3]{};
            std::int32_t syntheticModelLight[3]{};
            for (unsigned int axis = 0; axis < 3; ++axis) {
                std::int64_t native = 0;
                native += static_cast<std::int64_t>(
                    pCurrentMatrix->m[0][axis].asS32()) *
                    currentLightVector_M.lightVector[0].asS32();
                native += static_cast<std::int64_t>(
                    pCurrentMatrix->m[1][axis].asS32()) *
                    currentLightVector_M.lightVector[1].asS32();
                native += static_cast<std::int64_t>(
                    pCurrentMatrix->m[2][axis].asS32()) *
                    currentLightVector_M.lightVector[2].asS32();
                nativeModelLight[axis] =
                    static_cast<std::int32_t>(native >> 16);

                std::int64_t synthetic = 0;
                synthetic += static_cast<std::int64_t>(
                    g_pendingTownSubmission.state.modelMatrix[axis]) *
                    g_pendingTownSubmission.state.lightVector[0];
                synthetic += static_cast<std::int64_t>(
                    g_pendingTownSubmission.state.modelMatrix[4u + axis]) *
                    g_pendingTownSubmission.state.lightVector[1];
                synthetic += static_cast<std::int64_t>(
                    g_pendingTownSubmission.state.modelMatrix[8u + axis]) *
                    g_pendingTownSubmission.state.lightVector[2];
                syntheticModelLight[axis] =
                    static_cast<std::int32_t>(synthetic >> 16);
            }

            std::int32_t maxBasisDelta = 0;
            std::int32_t maxTranslationDelta = 0;
            for (unsigned int row = 0; row < 3; ++row) {
                for (unsigned int col = 0; col < 3; ++col) {
                    const unsigned int i = row * 4u + col;
                    const std::int32_t delta = static_cast<std::int32_t>(
                        std::llabs(
                            static_cast<long long>(actualWorld[i]) -
                            static_cast<long long>(
                                g_pendingTownSubmission.state.modelMatrix[i])));
                    maxBasisDelta = std::max(maxBasisDelta, delta);
                }
                const unsigned int i = row * 4u + 3u;
                const std::int32_t delta = static_cast<std::int32_t>(
                    std::llabs(
                        static_cast<long long>(actualWorld[i]) -
                        static_cast<long long>(
                            g_pendingTownSubmission.state.modelMatrix[i])));
                maxTranslationDelta =
                    std::max(maxTranslationDelta, delta);
            }

            const std::int32_t lightDelta0 =
                std::abs(nativeModelLight[0] - syntheticModelLight[0]);
            const std::int32_t lightDelta1 =
                std::abs(nativeModelLight[1] - syntheticModelLight[1]);
            const std::int32_t lightDelta2 =
                std::abs(nativeModelLight[2] - syntheticModelLight[2]);

            lagi::platform::logging::writef(
                "[FieldLightCompare] cell=%u obj=%08X model=%08X "
                "basisDelta=%d transDelta=%d "
                "nativeLight=(%d,%d,%d) synthLight=(%d,%d,%d) "
                "lightDelta=(%d,%d,%d)\n",
                g_pendingTownSubmission.cellIndex,
                g_pendingTownSubmission.objectIndex,
                g_pendingTownSubmission.modelTableOffset,
                maxBasisDelta,
                maxTranslationDelta,
                nativeModelLight[0],
                nativeModelLight[1],
                nativeModelLight[2],
                syntheticModelLight[0],
                syntheticModelLight[1],
                syntheticModelLight[2],
                lightDelta0,
                lightDelta1,
                lightDelta2);
            --g_staticLightingCompareBudget;
        }

        g_lastState = g_pendingTownSubmission.state;
        g_lastState.billboard = billboard;
    } else {
        capture_runtime_state(billboard);
    }

    std::int32_t adaptedIndex = -1;
    auto cached = g_modelCache.find(model);
    if (cached == g_modelCache.end()) {
        ++g_modelCacheMisses;
        LiveVdp1Model adapted{};
        if (adapt_processed_model(model, adapted))
            cached = g_modelCache.emplace(model, std::move(adapted)).first;
    }
    if (cached != g_modelCache.end()) {
        g_lastAdaptedModel = &cached->second;
        g_adaptedModels.push_back(g_lastAdaptedModel);
        adaptedIndex = static_cast<std::int32_t>(g_adaptedModels.size() - 1u);

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
