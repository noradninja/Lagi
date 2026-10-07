#include "lagi/platform.h"

unsigned char* getVdp1Pointer(unsigned int EA);
#include "lagi/debug_mesh.h"
#include "lagi/vdp1_renderer.h"
#include "lagi/lagi_town_runtime.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_live_model_adapter.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr/thread.h>
#include <psp2/kernel/threadmgr/semaphore.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <utility>
#include <vector>

namespace lagi::platform::renderer {

extern "C" {
extern const unsigned char _binary_lagi_color_v_gxp_start[];
extern const unsigned char _binary_lagi_color_f_gxp_start[];
extern const unsigned char _binary_lagi_texture_v_gxp_start[];
extern const unsigned char _binary_lagi_texture_f_gxp_start[];
extern const unsigned char _binary_lagi_mesh_f_gxp_start[];
extern const unsigned char _binary_lagi_cinepak_f_gxp_start[];
extern const unsigned char _binary_lagi_vdp2_nbg_f_gxp_start[];
extern const unsigned char _binary_lagi_vdp2_rbg0_f_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_payload_v_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_subdiv_v_gxp_start[];
extern const unsigned char _binary_lagi_textured_gouraud_subdiv_f_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_subdiv_gray_f_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_debug_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_lit_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_lit_opaque_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_lit_half_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_lit_newton_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_payload_probe_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_gouraud_noinverse_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_gouraud_noquant_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_rgb555_nogouraud_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_gouraud_finalquant_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_gouraud_scanline_f_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_scanline_gray_f_gxp_start[];
}

static constexpr int kWidth = 960;
static constexpr int kHeight = 544;
static constexpr int kPitch = 960;
// Lagi presents Saturn-era content directly; MSAA only adds bandwidth and
// fragment cost here, so keep every Neptune render path single-sampled.
static constexpr SceGxmMultisampleMode kMultisampleMode =
    SCE_GXM_MULTISAMPLE_NONE;
static constexpr std::size_t kFrameBytes =
    static_cast<std::size_t>(kPitch) * kHeight * sizeof(std::uint32_t);
static constexpr int kMaxStatus = 64;
static constexpr int kStatusRowsPerColumn = 32;
static constexpr int kStatusLineHeight = 14;
static constexpr int kStatusColumnX[2] = {40, 520};

struct StatusLine {
    char text[78];
    std::uint32_t color;
};

static SceUID g_frameMem[2] = { -1, -1 };
static std::uint32_t* g_frameBuffer[2] = { nullptr, nullptr };
static int g_drawBuffer = 0;

// One-slot game -> render pipeline. The game thread may simulate the next
// Azel frame while the render thread owns the previously published frame.
static SceUID g_renderThread = -1;
static SceUID g_renderFrameReadySema = -1;
static SceUID g_renderFrameFreeSema = -1;
static volatile bool g_renderThreadRunning = false;
static bool g_renderThreadStarted = false;
static unsigned int g_renderStartupFrame = 0;
static int renderThreadMain(SceSize args, void* argp);

static bool g_azelAlive = false;
static bool g_discAlive = false;
static bool g_debugVisible = true;
static StatusLine g_status[kMaxStatus]{};
static int g_statusCount = 0;
static bool g_suppressGxmInitPassStatus = false;
static bool g_gxmProbeAttempted = false;
static bool g_gxmInitialized = false;
static SceUID g_probeVdmUid = -1;
static SceUID g_probeVertexUid = -1;
static SceUID g_probeFragmentUid = -1;
static SceUID g_probeFragmentUsseUid = -1;
static void* g_probeVdm = nullptr;
static void* g_probeVertex = nullptr;
static void* g_probeFragment = nullptr;
static void* g_probeFragmentUsse = nullptr;
static SceGxmContext* g_probeContext = nullptr;
static void* g_probeContextHost = nullptr;
static SceGxmRenderTarget* g_probeRenderTarget = nullptr;
static SceGxmRenderTarget* g_probeRenderTargetHalf = nullptr;
// Movie/front-end presentation uses a dedicated 480x272 single-sample target.
// Cinepak is reconstructed at source resolution first, then sampled from an
// ordinary RGBA texture for the final bilinear presentation pass.
static SceGxmRenderTarget* g_movieRenderTarget = nullptr;
static SceGxmRenderTarget* g_frontendHighRenderTarget = nullptr;
static SceUID g_probeColorUid = -1;
static std::uint32_t* g_probeColorBuffer = nullptr;
static SceGxmColorSurface g_probeColorSurface{};
static SceGxmColorSurface g_probeColorSurfaceHalf{};
static SceGxmColorSurface g_movieColorSurface{};
static SceGxmColorSurface g_frontendHighColorSurface{};
static SceGxmSyncObject* g_probeSync = nullptr;
static SceUID g_probeColorUid2 = -1;
static std::uint32_t* g_probeColorBuffer2 = nullptr;
static SceGxmColorSurface g_probeColorSurface2{};
static SceGxmColorSurface g_probeColorSurfaceHalf2{};
static SceGxmColorSurface g_movieColorSurface2{};
static SceGxmColorSurface g_frontendHighColorSurface2{};
static SceGxmSyncObject* g_probeSync2 = nullptr;
static int g_gxmDrawBuffer = 1;
static SceUID g_probeDepthUid = -1;
static SceUID g_probeStencilUid = -1;
static void* g_probeDepth = nullptr;
static void* g_probeStencil = nullptr;
static SceGxmDepthStencilSurface g_probeDepthSurface{};
static SceGxmDepthStencilSurface g_probeDepthSurfaceHalf{};
static SceGxmShaderPatcher* g_probeShaderPatcher = nullptr;
static SceUID g_probePatcherBufferUid = -1;
static SceUID g_probePatcherVertexUsseUid = -1;
static SceUID g_probePatcherFragmentUsseUid = -1;
static void* g_probePatcherBuffer = nullptr;
static void* g_probePatcherVertexUsse = nullptr;
static void* g_probePatcherFragmentUsse = nullptr;
static SceGxmShaderPatcherId g_probeVertexProgramId{};
static SceGxmShaderPatcherId g_probeFragmentProgramId{};
static bool g_probeVertexRegistered = false;
static bool g_probeFragmentRegistered = false;
static SceGxmVertexProgram* g_probeVertexProgram = nullptr;
static SceGxmFragmentProgram* g_probeFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_fadeFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_colorOffsetAddFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_colorOffsetSubtractFragmentProgram = nullptr;
static const SceGxmProgramParameter* g_probeWvpParam = nullptr;
static SceUID g_fadeVertexUid = -1;
static SceUID g_fadeIndexUid = -1;
static azel::DebugColorVertex* g_fadeVertices = nullptr;
static std::uint16_t* g_fadeIndices = nullptr;

static SceGxmShaderPatcherId g_textureVertexProgramId{};
static SceGxmShaderPatcherId g_textureFragmentProgramId{};
static SceGxmShaderPatcherId g_cinepakFragmentProgramId{};
static SceGxmShaderPatcherId g_vdp2NbgFragmentProgramId{};
static SceGxmShaderPatcherId g_vdp2Rbg0FragmentProgramId{};
static bool g_textureVertexRegistered = false;
static bool g_textureFragmentRegistered = false;
static bool g_cinepakFragmentRegistered = false;
static bool g_vdp2NbgFragmentRegistered = false;
static bool g_vdp2Rbg0FragmentRegistered = false;
static bool g_vdp2Rbg0Available = false;
static SceGxmVertexProgram* g_textureVertexProgram = nullptr;
static SceGxmFragmentProgram* g_textureFragmentProgram = nullptr;
// Movie/front-end variants are explicitly single-sample. The normal texture
// program uses kMultisampleMode, which is also single-sample globally.
static SceGxmFragmentProgram* g_movieTextureFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_cinepakFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_vdp2NbgFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_vdp2Rbg0FragmentProgram = nullptr;
static const SceGxmProgramParameter* g_cinepakMovieInfoParam = nullptr;
static const SceGxmProgramParameter* g_vdp2InfoParam = nullptr;
static const SceGxmProgramParameter* g_vdp2Rbg0PlaneParam[4] = {
    nullptr, nullptr, nullptr, nullptr
};
static const SceGxmProgramParameter* g_vdp2Rbg0Transform0Param = nullptr;
static const SceGxmProgramParameter* g_vdp2Rbg0Transform1Param = nullptr;
static const SceGxmProgramParameter* g_vdp2Rbg0CoefficientParam = nullptr;
static const SceGxmProgramParameter* g_vdp2Rbg0InfoParam = nullptr;
static const SceGxmProgramParameter* g_vdp2Rbg0FormatParam = nullptr;
static SceGxmShaderPatcherId g_meshFragmentProgramId{};
static bool g_meshFragmentRegistered = false;
static SceGxmFragmentProgram* g_meshTextureFragmentProgram = nullptr;
static SceGxmFragmentProgram* g_meshSubdivFragmentProgram = nullptr;
static SceGxmShaderPatcherId g_gouraudPayloadVertexProgramId{};
static bool g_gouraudPayloadVertexRegistered = false;
static SceGxmVertexProgram* g_gouraudPayloadVertexProgram = nullptr;
static const SceGxmProgramParameter* g_gouraudPayloadWvpParam = nullptr;

static SceGxmShaderPatcherId g_gouraudSubdivVertexProgramId{};
static bool g_gouraudSubdivVertexRegistered = false;
static SceGxmVertexProgram* g_gouraudSubdivVertexProgram = nullptr;
static const SceGxmProgramParameter* g_gouraudSubdivWvpParam = nullptr;
static SceGxmShaderPatcherId g_texturedGouraudSubdivFragmentProgramId{};
static bool g_texturedGouraudSubdivFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedGouraudSubdivFragmentProgram = nullptr;
static SceGxmShaderPatcherId g_gouraudSubdivGrayFragmentProgramId{};
static bool g_gouraudSubdivGrayFragmentRegistered = false;
static SceGxmFragmentProgram* g_gouraudSubdivGrayFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_gouraudDebugFragmentProgramId{};
static bool g_gouraudDebugFragmentRegistered = false;
static SceGxmFragmentProgram* g_gouraudDebugFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedLitFragmentProgramId{};
static bool g_texturedLitFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedLitFragmentProgram = nullptr;
static SceGxmShaderPatcherId g_texturedLitOpaqueFragmentProgramId{};
static bool g_texturedLitOpaqueFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedLitOpaqueFragmentProgram = nullptr;
static SceGxmShaderPatcherId g_texturedLitHalfFragmentProgramId{};
static bool g_texturedLitHalfFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedLitHalfFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedLitNewtonFragmentProgramId{};
static bool g_texturedLitNewtonFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedLitNewtonFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedPayloadProbeFragmentProgramId{};
static bool g_texturedPayloadProbeFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedPayloadProbeFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedGouraudNoInverseFragmentProgramId{};
static bool g_texturedGouraudNoInverseFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedGouraudNoInverseFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedGouraudNoQuantFragmentProgramId{};
static bool g_texturedGouraudNoQuantFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedGouraudNoQuantFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedRgb555NoGouraudFragmentProgramId{};
static bool g_texturedRgb555NoGouraudFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedRgb555NoGouraudFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedGouraudFinalQuantFragmentProgramId{};
static bool g_texturedGouraudFinalQuantFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedGouraudFinalQuantFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedGouraudScanlineFragmentProgramId{};
static bool g_texturedGouraudScanlineFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedGouraudScanlineFragmentProgram = nullptr;
static SceGxmShaderPatcherId g_gouraudScanlineGrayFragmentProgramId{};
static bool g_gouraudScanlineGrayFragmentRegistered = false;
static SceGxmFragmentProgram* g_gouraudScanlineGrayFragmentProgram = nullptr;

static const SceGxmProgramParameter* g_textureWvpParam = nullptr;
static bool g_probeScenePassed = false;
static azel::BasicWingDebugMesh g_basicWingCpuMesh{};
static bool g_basicWingCpuReady = false;
static azel::StaticRoomDebugMesh g_staticRoomCpuMesh{};
static bool g_staticRoomCpuReady = false;
static azel::BasicWingDebugMesh g_edgeIdleCpuMesh{};
static bool g_edgeIdleCpuReady = false;
static azel::BasicWingDebugMesh g_edgeShadowCpuMesh{};
static bool g_edgeShadowCpuReady = false;
static std::vector<std::uint16_t> g_edgeShadowTownTextureIndices;
static azel::BasicWingDebugMesh g_liveTownCpuMesh{};

struct LivePolygonLightState {
    std::int32_t vector[3]{};
    std::uint16_t color[3]{};
    std::uint32_t falloff[3]{};
    bool valid = false;
};
static std::vector<LivePolygonLightState> g_liveTownPolygonLights;

static std::uint64_t g_liveTownSignature = 0;
static std::uint64_t g_liveTownStaticSignature = 0;
static std::size_t g_liveTownStaticVertexCount = 0;
static std::size_t g_liveTownStaticPolygonCount = 0;
static bool g_liveTownStaticRebuilt = true;

static std::size_t g_liveTownShadowFirstPolygon = 0;
static std::size_t g_liveTownShadowPolygonCount = 0;
static std::size_t g_liveTownEdgeFirstPolygon = 0;
static std::size_t g_liveTownEdgePolygonCount = 0;

// Authentic Azel mesh-mode primitives (CMDPMOD bit 8) need Saturn-style
// ordered overdraw rather than ordinary z-buffer ownership. Track their
// source polygon ranges generically; do not attach object identity here.
struct LiveTownMeshRange {
    std::size_t first = 0;
    std::size_t count = 0;
};
static std::vector<LiveTownMeshRange> g_liveTownMeshRanges;
static bool g_liveTownPrepared = false;
static std::size_t g_edgeFirstVertex = 0;
static std::size_t g_edgeFirstPolygon = 0;

// Mode 9 submission diagnostics. Azel still owns town cell/object visibility;
// these counters describe only the final backend primitive stream.
static unsigned int g_authFlatTotalQuads = 0;
static unsigned int g_authFlatVisibleQuads = 0;
static unsigned int g_authFlatDrawCalls = 0;

// Live-town cache diagnostics. These observe Azel's existing submissions;
// they do not participate in visibility or scene ownership.
static unsigned int g_liveTownSubmissionCount = 0;
static unsigned int g_liveTownStaticSubmissionCount = 0;
static unsigned int g_liveTownBillboardSubmissionCount = 0;
static unsigned int g_liveTownStaticSubmittedPolygons = 0;
static unsigned int g_liveTownBillboardSubmittedPolygons = 0;
static bool g_liveTownHasBillboards = false;

struct LiveTownResolvedMaterialCache {
    sProcessed3dModel* model = nullptr;
    std::vector<std::uint16_t> textureIndices;
};
static std::vector<LiveTownResolvedMaterialCache> g_liveTownMaterialCache;

// Frame profiler samples are microseconds. TASK is written after runTasks(),
// so the HUD naturally shows the previous task frame while the render samples
// describe the render currently being presented.
static unsigned int g_profileTasksUs = 0;
static unsigned int g_profileGameWaitUs = 0;
static unsigned int g_profileRenderCpuPrepUs = 0;
static unsigned int g_profileBuildUs = 0;
static unsigned int g_profileBuildScanUs = 0;
static unsigned int g_profileBuildCacheUs = 0;
static unsigned int g_profileBuildEdgeUs = 0;
static unsigned int g_profileBuildValidateUs = 0;
static unsigned int g_profileBuildUploadUs = 0;
static unsigned int g_profilePrepareReleaseUs = 0;
static unsigned int g_profilePrepareBaseAllocUs = 0;
static unsigned int g_profilePrepareWireUs = 0;
static unsigned int g_profilePrepareTextureUploadUs = 0;
static unsigned int g_profilePrepareTexturedAllocUs = 0;
static unsigned int g_profilePrepareTexturedBuildUs = 0;
static unsigned int g_profilePrepareSubdivAllocUs = 0;
static unsigned int g_profilePrepareSubdivBuildUs = 0;
static unsigned int g_profilePrepareCopyUs = 0;
static bool g_profilePrepareReusedTextures = false;
static bool g_profilePrepareReusedGeometry = false;
static unsigned int g_profileEdgeCopyUs = 0;
static unsigned int g_profileEdgeAnimUs = 0;
static unsigned int g_profileEdgeAppendUs = 0;
static unsigned int g_profileObjectAppendUs = 0;
static unsigned int g_profileObjectMaterialResolveUs = 0;
static unsigned int g_profileObjectMaterialCacheMisses = 0;
static unsigned int g_profileLightingUs = 0;
static unsigned int g_profileSubmitUs = 0;
static unsigned int g_profileGouraudProjectUs = 0;
static unsigned int g_profileGouraudPayloadUs = 0;
static unsigned int g_profileGouraudBucketUs = 0;
static unsigned int g_profileGouraudIndexUs = 0;
static unsigned int g_profileGouraudDrawUs = 0;
static unsigned int g_profileGouraudVisibleQuads = 0;
static unsigned int g_profileGouraudTotalQuads = 0;
static unsigned int g_profileGouraudPrepUs = 0;
static unsigned int g_profileGxmWaitUs = 0;
static unsigned int g_profileRenderUs = 0;
static unsigned int g_profilePresentUs = 0;

enum class ResidentVdp1Model {
    None,
    BasicWing,
    StaticRoomDiagnostic,
    LiveTown,
};
static ResidentVdp1Model g_residentVdp1Model = ResidentVdp1Model::None;
static SceUID g_vdp1VertexUid = -1;
static SceUID g_vdp1LightingVertexUid = -1;
static SceUID g_vdp1IndexUid = -1;
static azel::DebugColorVertex* g_vdp1Vertices = nullptr;
static azel::DebugColorVertex* g_vdp1LightingVertices = nullptr;
static std::uint16_t* g_vdp1Indices = nullptr;
static unsigned int g_vdp1VertexCapacity = 0u;
static unsigned int g_vdp1IndexCapacity = 0u;

static SceUID g_vdp1TextureVertexUid = -1;
static SceUID g_vdp1GouraudVertexUid = -1;
static SceUID g_vdp1TextureIndexUid = -1;
static azel::DebugTextureVertex* g_vdp1TextureVertices = nullptr;
static azel::DebugGouraudPayloadVertex* g_vdp1GouraudVertices = nullptr;
static std::uint16_t* g_vdp1TextureIndices = nullptr;
static unsigned int g_vdp1TextureVertexCapacity = 0u;
static unsigned int g_vdp1TextureIndexCapacity = 0u;

struct SubdivGouraudVertex {
    float x, y, z;
    float u, v;
    float shadeR, shadeG, shadeB;
};
static SceUID g_vdp1SubdivVertexUid = -1;
static SceUID g_vdp1SubdivIndexUid = -1;
static SubdivGouraudVertex* g_vdp1SubdivVertices = nullptr;
static std::uint16_t* g_vdp1SubdivIndices = nullptr;
static unsigned int g_vdp1SubdivVertexCapacity = 0u;
static unsigned int g_vdp1SubdivIndexCapacity = 0u;
static std::vector<std::uint16_t> g_vdp1SubdivQuadIndices;

static SceUID g_vdp1SubdivWireVertexUid = -1;
static SceUID g_vdp1SubdivWireIndexUid = -1;
static azel::DebugColorVertex* g_vdp1SubdivWireVertices = nullptr;
static std::uint16_t* g_vdp1SubdivWireIndices = nullptr;
static unsigned int g_vdp1SubdivWireVertexCapacity = 0u;
static unsigned int g_vdp1SubdivWireIndexCapacity = 0u;

struct TextureBatch {
    unsigned int firstIndex = 0;
    unsigned int indexCount = 0;
};

struct GpuMode1Texture {
    SceUID uid = -1;
    void* data = nullptr;
    SceGxmTexture texture{};
    unsigned int width = 0;
    unsigned int height = 0;
    bool opaque = false;
    bool mesh = false;
};

static std::vector<TextureBatch> g_vdp1TextureBatches;
static std::vector<GpuMode1Texture> g_vdp1GpuTextures;
static bool g_vdp1TexturedReady = false;
static bool g_vdp1TextureDataDirty = false;

// Lightweight VDP1 2D/UI translator. These resources are separate from the
// resident 3D town mesh so UI commands never invalidate or rebuild it.
struct Vdp1UiTextureCacheEntry {
    std::uint16_t cmdPmod = 0;
    std::uint16_t cmdColr = 0;
    std::uint16_t cmdSrca = 0;
    std::uint16_t cmdSize = 0;
    GpuMode1Texture gpu{};
};
static std::vector<Vdp1UiTextureCacheEntry> g_vdp1UiTextureCache;
static SceUID g_vdp1UiVertexUid = -1;
static SceUID g_vdp1UiIndexUid = -1;
static azel::DebugTextureVertex* g_vdp1UiVertices = nullptr;
static std::uint16_t* g_vdp1UiIndices = nullptr;
static SceUID g_vdp1UiLineVertexUid = -1;
static SceUID g_vdp1UiLineIndexUid = -1;
static azel::DebugColorVertex* g_vdp1UiLineVertices = nullptr;
static std::uint16_t* g_vdp1UiLineIndices = nullptr;

static constexpr unsigned int kVdp2Nbg1AtlasWidth = 128u;
static constexpr unsigned int kVdp2Nbg1AtlasHeight = 128u;
static constexpr unsigned int kVdp2Nbg1MaxTiles = 64u;
static constexpr unsigned int kVdp2Nbg1MaxCells = 32u * 14u;
static SceUID g_vdp2Nbg1AtlasUid = -1;
static std::uint32_t* g_vdp2Nbg1AtlasPixels = nullptr;
static SceGxmTexture g_vdp2Nbg1AtlasTexture{};
static SceUID g_vdp2Nbg1VertexUid = -1;
static SceUID g_vdp2Nbg1IndexUid = -1;
static azel::DebugTextureVertex* g_vdp2Nbg1Vertices = nullptr;
static std::uint16_t* g_vdp2Nbg1Indices = nullptr;

static constexpr unsigned int kVdp2TextLayerWidth = 352u;
static constexpr unsigned int kVdp2TextLayerHeight = 224u;
static SceUID g_vdp2TextLayerUid = -1;
static std::uint32_t* g_vdp2TextLayerPixels = nullptr;
static SceGxmTexture g_vdp2TextLayerTexture{};
static SceUID g_vdp2TextLayerVertexUid = -1;
static SceUID g_vdp2TextLayerIndexUid = -1;
static azel::DebugTextureVertex* g_vdp2TextLayerVertices = nullptr;
static std::uint16_t* g_vdp2TextLayerIndices = nullptr;
static SceUID g_vdp2BarVertexUid = -1;
static SceUID g_vdp2BarIndexUid = -1;
static azel::DebugColorVertex* g_vdp2BarVertices = nullptr;
static std::uint16_t* g_vdp2BarIndices = nullptr;

// Phase 1 movie presentation owns a separate dynamic RGBA texture. It does
// not invalidate Neptune's resident VDP1 town mesh, VDP1 UI cache, VDP2 text
// atlas, cinematic bars, or fade resources.
static SceUID g_movieTextureUid = -1;
static SceUID g_movieVertexUid = -1;
static SceUID g_movieIndexUid = -1;
static void* g_movieTextureData = nullptr;
static azel::DebugTextureVertex* g_movieVertices = nullptr;
static std::uint16_t* g_movieIndices = nullptr;
static SceUID g_vdp2WindowVertexUid = -1;
static SceUID g_vdp2WindowIndexUid = -1;
static azel::DebugTextureVertex* g_vdp2WindowVertices = nullptr;
static std::uint16_t* g_vdp2WindowIndices = nullptr;
static SceGxmTexture g_movieTexture{};

// Cinepak's compact payload cannot be filtered directly because its texels
// contain codebook/mode data rather than display pixels. Reconstruct it into
// this 480x272-capable RGBA surface at the movie's native source dimensions,
// then bilinear-filter that conventional image during final presentation.
static SceUID g_cinepakResolveColorUid = -1;
static std::uint32_t* g_cinepakResolveColorBuffer = nullptr;
static SceGxmColorSurface g_cinepakResolveColorSurface{};
static SceGxmTexture g_cinepakResolveTexture{};
static SceUID g_cinepakResolveVertexUid = -1;
static SceUID g_cinepakResolveIndexUid = -1;
static azel::DebugTextureVertex* g_cinepakResolveVertices = nullptr;
static std::uint16_t* g_cinepakResolveIndices = nullptr;

// Title NBG0 is decoded once from Azel's exact VRAM/CRAM representation into
// a conventional RGBA surface. Raw Saturn memory remains point-exact during
// decode; SGX performs only the final presentation-scale bilinear sample.
static constexpr unsigned int kTitleDecodedWidth = 704u;
static constexpr unsigned int kTitleDecodedHeight = 448u;
static SceUID g_titleDecodedUid = -1;
static std::uint32_t* g_titleDecodedPixels = nullptr;
static SceGxmTexture g_titleDecodedTexture{};
static bool g_titleDecodedValid = false;

static unsigned int g_movieWidth = 0;
static unsigned int g_movieHeight = 0;
static unsigned int g_movieStridePixels = 0;
static unsigned int g_moviePayloadWidth = 0;
static unsigned int g_moviePayloadHeight = 0;
static bool g_movieUsesCinepakPayload = false;
static bool g_movieUsesVdp2Title = false;
static bool g_movieFrameVisible = false;
static float g_movieVdp2Info[4] = {};
static unsigned int g_movieVdp2Tvmd = 0u;
static float g_movieRbg0Planes[16] = {};
static float g_movieRbg0PlanesB[16] = {};
static float g_movieRbg0Ctrl[16] = {};
static float g_movieRbg0Plsz = 0.0f;
static float g_movieRbg0Format[4] = {};
static float g_movieRbg0TransformA[8] = {};
static float g_movieRbg0TransformB[8] = {};
static float g_movieRbg0CoefficientA[4] = {};
static float g_movieRbg0CoefficientB[4] = {};
static std::atomic<unsigned int> g_azelColorOffsetEnable{0};
static std::atomic<unsigned int> g_azelColorOffsetSelect{0};
static std::atomic<int> g_azelColorOffsetARed{0};
static std::atomic<int> g_azelColorOffsetAGreen{0};
static std::atomic<int> g_azelColorOffsetABlue{0};
static std::atomic<int> g_azelColorOffsetBRed{0};
static std::atomic<int> g_azelColorOffsetBGreen{0};
static std::atomic<int> g_azelColorOffsetBBlue{0};
static SceUID g_movieFrameSema = -1;
static bool g_movieUploadLogged = false;
static bool g_movieRenderLogged = false;

class MovieFrameGuard {
public:
    MovieFrameGuard()
    {
        locked_ = g_movieFrameSema >= 0 &&
            sceKernelWaitSema(g_movieFrameSema, 1, nullptr) >= 0;
    }

    ~MovieFrameGuard()
    {
        if (locked_)
            sceKernelSignalSema(g_movieFrameSema, 1);
    }

    explicit operator bool() const { return locked_; }

private:
    bool locked_ = false;
};

class MovieRenderSlotGuard {
public:
    MovieRenderSlotGuard()
    {
        if (!g_renderThreadStarted || g_renderFrameFreeSema < 0 ||
            g_renderFrameReadySema < 0) {
            acquired_ = true;
            return;
        }

        acquired_ =
            sceKernelWaitSema(g_renderFrameFreeSema, 1, nullptr) >= 0;
        ownsFreeSlot_ = acquired_;
    }

    ~MovieRenderSlotGuard()
    {
        if (ownsFreeSlot_)
            sceKernelSignalSema(g_renderFrameFreeSema, 1);
    }

    explicit operator bool() const { return acquired_; }

    bool publish()
    {
        if (!acquired_)
            return false;
        if (!ownsFreeSlot_)
            return true;

        if (sceKernelSignalSema(g_renderFrameReadySema, 1) < 0)
            return false;

        // The render thread now owns the slot and will return it after the
        // submitted movie frame has finished using renderer-owned state.
        ownsFreeSlot_ = false;
        return true;
    }

private:
    bool acquired_ = false;
    bool ownsFreeSlot_ = false;
};

static bool g_probeDisplayingGxm = false;
static bool g_viewerReady = false;
static float g_viewYaw = 0.0f;
static float g_viewPitch = 0.0f;
static float g_viewDistance = 3.0f;

static float g_basicWingViewCenter[3]{};
static float g_basicWingFitDistance = 3.0f;
static float g_staticRoomViewCenter[3]{};
static float g_staticRoomFitDistance = 3.0f;

// First playable-town runtime slice. The recovered Edge transform is kept
// mutable here instead of baking movement into the reconstructed room mesh.
static bool g_townPlayerReady = false;
static bool g_townCameraReady = false;
static unsigned int g_sceneGameMode = 0u;
static float g_townPlayerPosition[3]{};
static float g_townPlayerYaw = 0.0f;
static float g_townCameraPosition[3]{};
static float g_townCameraRawPosition[3]{};
static float g_townCameraTarget[3]{};
static float g_townCameraUp[3]{};
static float g_townCameraYaw = 0.0f;
static float g_townCameraPitch = 0.0f;
static float g_townCameraDistance = 0.0f;
static float g_nativeSceneNearPlane = 0.0f;
static float g_nativeSceneFarPlane = 0.0f;
static float g_azelProjectionFovDegrees = 80.0f;
static bool g_townPlayerGrounded = false;
static unsigned int g_townCollisionContacts = 0;
static unsigned int g_townEdgeAnimation = 0;
static unsigned int g_townEdgeAnimationFrame = 0;
static unsigned int g_townEdgePreviousAnimation = 0;
static unsigned int g_townEdgePreviousFrame = 0;
static float g_townEdgeTransition = 1.0f;

// Game-thread presentation staging. presentation_set_* only writes these fields.
// presentation_publish_frame() atomically defines the serial frame boundary by copying
// them into the renderer-owned state above. This avoids a future render thread
// reading task-owned state while the next Azel frame is mutating it.
static float g_pendingTownPlayerPosition[3]{};
static float g_pendingTownPlayerYaw = 0.0f;
static float g_pendingTownCameraPosition[3]{};
static float g_pendingTownCameraRawPosition[3]{};
static float g_pendingTownCameraTarget[3]{};
static float g_pendingTownCameraUp[3]{};
static float g_pendingTownCameraYaw = 0.0f;
static float g_pendingTownCameraPitch = 0.0f;
static float g_pendingTownCameraDistance = 0.0f;
static float g_pendingNativeSceneNearPlane = 0.0f;
static float g_pendingNativeSceneFarPlane = 0.0f;
static bool g_pendingTownPlayerGrounded = false;
static unsigned int g_pendingTownCollisionContacts = 0;
static unsigned int g_pendingTownEdgeAnimation = 0;
static unsigned int g_pendingTownEdgeAnimationFrame = 0;
static unsigned int g_pendingTownEdgePreviousAnimation = 0;
static unsigned int g_pendingTownEdgePreviousFrame = 0;
static float g_pendingTownEdgeTransition = 1.0f;
static bool g_pendingTownPresentationValid = false;
static unsigned int g_pendingSceneGameMode = 0u;
static int g_pendingViewMode = 7;
static unsigned int g_pendingProfileTasksUs = 0u;
static unsigned int g_pendingProfileGameWaitUs = 0u;

static constexpr std::size_t kVdp2TextSnapshotBytes = 0x10000u;
static constexpr std::size_t kVdp2CramSnapshotBytes = 0x1000u;
static std::uint8_t g_pendingVdp2TextVram[kVdp2TextSnapshotBytes]{};
static std::uint8_t g_pendingVdp2Cram[kVdp2CramSnapshotBytes]{};
static std::uint8_t g_vdp2TextVram[kVdp2TextSnapshotBytes]{};
static std::uint8_t g_vdp2Cram[kVdp2CramSnapshotBytes]{};
static bool g_pendingVdp2TextValid = false;
static bool g_vdp2TextValid = false;

static constexpr std::size_t kVdp2LineScrollBytes = 0x400u;
static std::uint8_t g_pendingVdp2LineScroll[kVdp2LineScrollBytes]{};
static std::uint8_t g_vdp2LineScroll[kVdp2LineScrollBytes]{};

// Script-owned town fade command. The game thread stages commands here; the
// completed-frame publish copies them across the render handoff. This remains
// separate from VDP2 color offset because Azel's town camera fade is a
// full-presentation effect, not CLOFEN layer selection.
static unsigned int g_pendingTownFadeSerial = 0u;
static bool g_pendingTownFadeIn = false;
static unsigned int g_pendingTownFadeFrames = 1u;

static unsigned int g_townFadeSerial = 0u;
static unsigned int g_townFadeAppliedSerial = 0u;
static bool g_townFadeIn = false;
static unsigned int g_townFadeFrames = 1u;
static unsigned int g_townFadeElapsed = 0u;
static float g_townFadeBlack = 1.0f;

static int g_viewMode = 7;
static constexpr bool g_halfResolution = true;
// Compact timing HUD used for capture/video analysis of the game/render split.
// Hidden by default; Select toggles it at runtime.
static bool g_showThreadTimingOsd = false;
// Retain the larger legacy diagnostics in code, but keep them inaccessible
// during normal play. Logs/status collection remain active.
static constexpr bool kShowTownDiagnostics = false;
static unsigned int g_basicWingAnimationFrame = 0;

static Vdp1ModelSource basicWingVdp1Source()
{
    Vdp1ModelSource source{};
    source.vertices = g_basicWingCpuMesh.vertices.data();
    source.lightingVertices = g_basicWingCpuMesh.lightingVertices.data();
    source.vertexCount = g_basicWingCpuMesh.vertices.size();
    source.polygons = g_basicWingCpuMesh.polygonRecords.data();
    source.gouraud555 = g_basicWingCpuMesh.gouraud555.data();
    source.polygonCount = g_basicWingCpuMesh.polygons;
    source.textures = g_basicWingCpuMesh.decodedTextureData.data();
    source.textureCount = g_basicWingCpuMesh.decodedTextureData.size();
    source.polygonTextureIndices = g_basicWingCpuMesh.polygonTextureIndices.data();
    source.polygonTextureIndexCount = g_basicWingCpuMesh.polygonTextureIndices.size();
    return source;
}

static Vdp1ModelSource staticRoomVdp1Source(bool authenticCamera)
{
    Vdp1ModelSource source{};
    source.vertices = authenticCamera
        ? g_staticRoomCpuMesh.worldVertices.data()
        : g_staticRoomCpuMesh.vertices.data();
    source.lightingVertices = authenticCamera
        ? g_staticRoomCpuMesh.worldLightingVertices.data()
        : g_staticRoomCpuMesh.lightingVertices.data();
    source.vertexCount = authenticCamera
        ? g_staticRoomCpuMesh.worldVertices.size()
        : g_staticRoomCpuMesh.vertices.size();
    source.polygons = g_staticRoomCpuMesh.polygonRecords.data();
    source.gouraud555 = g_staticRoomCpuMesh.gouraud555.data();
    source.polygonCount = g_staticRoomCpuMesh.polygons;

    if (g_staticRoomCpuMesh.texturesFullyResolved) {
        source.textures = g_staticRoomCpuMesh.decodedTextureData.data();
        source.textureCount = g_staticRoomCpuMesh.decodedTextureData.size();
        source.polygonTextureIndices =
            g_staticRoomCpuMesh.polygonTextureIndices.data();
        source.polygonTextureIndexCount =
            g_staticRoomCpuMesh.polygonTextureIndices.size();
    }

    return source;
}

static bool g_presentClockInitialized = false;
static unsigned int g_lastPresentVcount = 0;

// Defined below with the textured-viewer helpers; shutdown() needs it earlier.
static void freeVdp1Textures();
static void freeMovieResources();
static void updateLiveTownAzelLighting();
static bool ensureVdp2UiGpuBuffers();
static int viewerRenderWidth();
static int viewerRenderHeight();
static int viewerRenderPitch();

static void waitFor30HzPresentSlot()
{
    // The Vita display scans at 60 Hz. Queueing with NEXTFRAME makes the
    // buffer become front on the following vblank, so wait until at least
    // one vblank has elapsed since the previous presentation before queueing.
    // The queued buffer then lands on the second vblank: a stable 30 Hz cap.
    //
    // If rendering itself already consumed one or more vblanks, this does
    // not add an unnecessary fixed two-vblank delay.
    unsigned int now =
        static_cast<unsigned int>(sceDisplayGetVcount());

    if (!g_presentClockInitialized) {
        g_lastPresentVcount = now;
        g_presentClockInitialized = true;
    }

    while (static_cast<unsigned int>(
               now - g_lastPresentVcount) < 1u) {
        sceDisplayWaitVblankStart();
        now = static_cast<unsigned int>(
            sceDisplayGetVcount());
    }
}

static void mark30HzPresented()
{
    g_lastPresentVcount =
        static_cast<unsigned int>(sceDisplayGetVcount());
    g_presentClockInitialized = true;
}

static void fill(std::uint32_t color)
{
    std::uint32_t* buffer = g_frameBuffer[g_drawBuffer];
    if (!buffer) return;
    for (int y = 0; y < kHeight; ++y) {
        std::uint32_t* row = buffer + y * kPitch;
        for (int x = 0; x < kWidth; ++x) row[x] = color;
    }
}

static const std::uint8_t* glyph(char c)
{
    static const std::uint8_t blank[7] = {0,0,0,0,0,0,0};
    static const std::uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
    };
    static const std::uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}
    };
    static const std::uint8_t colon[7] = {0,4,4,0,4,4,0};
    static const std::uint8_t dash[7]  = {0,0,0,31,0,0,0};
    static const std::uint8_t slash[7] = {1,2,2,4,8,8,16};
    static const std::uint8_t dot[7]   = {0,0,0,0,0,6,6};
    static const std::uint8_t lbr[7]   = {14,8,8,8,8,8,14};
    static const std::uint8_t rbr[7]   = {14,2,2,2,2,2,14};

    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c == ':') return colon;
    if (c == '-') return dash;
    if (c == '/') return slash;
    if (c == '.') return dot;
    if (c == '[') return lbr;
    if (c == ']') return rbr;
    return blank;
}

static void drawChar(int x, int y, char c, std::uint32_t color, int scale = 2)
{
    const std::uint8_t* rows = glyph(c);
    for (int gy = 0; gy < 7; ++gy) {
        for (int gx = 0; gx < 5; ++gx) {
            if (!(rows[gy] & (1u << (4 - gx)))) continue;
            for (int sy = 0; sy < scale; ++sy) {
                const int py = y + gy * scale + sy;
                if (py < 0 || py >= kHeight) continue;
                for (int sx = 0; sx < scale; ++sx) {
                    const int px = x + gx * scale + sx;
                    if (px >= 0 && px < kWidth)
                        g_frameBuffer[g_drawBuffer][py * kPitch + px] = color;
                }
            }
        }
    }
}

static void drawText(int x, int y, const char* text, std::uint32_t color, int scale = 2)
{
    if (!text) return;
    const int advance = 6 * scale;
    for (const char* p = text; *p; ++p) {
        drawChar(x, y, *p, color, scale);
        x += advance;
        if (x > kWidth - advance) break;
    }
}

// 4:3 nearest-neighbour enlargement for the 5x7 debug font. This makes the
// scale-1 UI roughly 33% larger without changing the scale-2 title font.
static void drawCharSmall(int x, int y, char c, std::uint32_t color)
{
    const std::uint8_t* rows = glyph(c);
    for (int gy = 0; gy < 7; ++gy) {
        const int py = y + gy;
        if (py < 0 || py >= kHeight)
            continue;
        for (int gx = 0; gx < 5; ++gx) {
            if (!(rows[gy] & (1u << (4 - gx))))
                continue;
            const int px = x + gx;
            if (px >= 0 && px < kWidth)
                g_frameBuffer[g_drawBuffer][py * kPitch + px] = color;
        }
    }
}

static void drawTextSmall(int x, int y, const char* text, std::uint32_t color)
{
    if (!text) return;
    static constexpr int advance = 6;
    for (const char* p = text; *p; ++p) {
        drawCharSmall(x, y, *p, color);
        x += advance;
        if (x > kWidth - advance)
            break;
    }
}

static void drawCharSmallToBuffer(
    std::uint32_t* buffer,
    int pitch,
    int x,
    int y,
    char c,
    std::uint32_t color)
{
    if (!buffer)
        return;

    const std::uint8_t* rows = glyph(c);
    const int width = g_halfResolution ? (kWidth / 2) : kWidth;
    const int height = g_halfResolution ? (kHeight / 2) : kHeight;
    for (int gy = 0; gy < 7; ++gy) {
        const int py = y + gy;
        if (py < 0 || py >= height)
            continue;
        for (int gx = 0; gx < 5; ++gx) {
            if (!(rows[gy] & (1u << (4 - gx))))
                continue;
            const int px = x + gx;
            if (px >= 0 && px < width)
                buffer[py * pitch + px] = color;
        }
    }
}

static void drawTextSmallToBuffer(
    std::uint32_t* buffer,
    int pitch,
    int x,
    int y,
    const char* text,
    std::uint32_t color)
{
    if (!buffer || !text)
        return;

    static constexpr int advance = 6;
    const int width = g_halfResolution ? (kWidth / 2) : kWidth;
    for (const char* p = text; *p; ++p) {
        drawCharSmallToBuffer(buffer, pitch, x, y, *p, color);
        x += advance;
        if (x > width - advance)
            break;
    }
}

static int viewerRenderWidth();
static int viewerRenderHeight();

static const char* viewerModeLabel(int mode)
{
    switch (mode) {
    case 7:  return "FULL";
    case 8:  return "TEXTURE";
    case 10: return "LIGHTING";
    case 9:  return "QUADS";
    case 11: return "WIRES";
    default: return "";
    }
}

static void drawViewerModeOverlay(
    std::uint32_t* buffer,
    int pitch,
    int mode)
{
    const char* label = viewerModeLabel(mode);
    const int length =
        static_cast<int>(std::strlen(label));
    static constexpr int advance = 6;
    static constexpr int marginX = 8;
    static constexpr int marginY = 8;
    static constexpr int textHeight = 7;

    const int x =
        std::max(
            0,
            viewerRenderWidth() - marginX - length * advance);
    const int y =
        viewerRenderHeight() - marginY - textHeight;

    // One-pixel black offset gives the text enough contrast over bright
    // textures without adding a large opaque diagnostic panel.
    drawTextSmallToBuffer(
        buffer, pitch, x + 1, y + 1,
        label, 0xFF000000u);
    drawTextSmallToBuffer(
        buffer, pitch, x, y,
        label, 0xFFFFFFFFu);
}

// Forward declarations used by the decoded title cache helpers below.
static void* probeGpuAlloc(
    unsigned int size,
    unsigned int attribs,
    SceUID* uid);
static void freeMovieMappedBlock(SceUID& uid, void*& memory);
static std::uint32_t vdp2Rgb555ToAbgr(std::uint16_t color);

static std::uint16_t readVdp2Be16(
    const std::uint8_t* bytes,
    std::size_t offset)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8) |
        static_cast<std::uint16_t>(bytes[offset + 1]));
}


static bool ensureTitleDecodedTexture()
{
    if (g_titleDecodedPixels)
        return true;

    const unsigned int bytes =
        kTitleDecodedWidth * kTitleDecodedHeight *
        sizeof(std::uint32_t);
    g_titleDecodedPixels = static_cast<std::uint32_t*>(
        probeGpuAlloc(
            bytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_titleDecodedUid));
    if (!g_titleDecodedPixels)
        return false;

    if (sceGxmTextureInitLinear(
            &g_titleDecodedTexture,
            g_titleDecodedPixels,
            SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
            kTitleDecodedWidth,
            kTitleDecodedHeight,
            0) < 0) {
        void* pixels = g_titleDecodedPixels;
        freeMovieMappedBlock(g_titleDecodedUid, pixels);
        g_titleDecodedPixels = nullptr;
        return false;
    }

    sceGxmTextureSetMinFilter(
        &g_titleDecodedTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);
    sceGxmTextureSetMagFilter(
        &g_titleDecodedTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);
    return true;
}

static bool updateTitleDecodedTexture(
    const unsigned char* vram,
    const unsigned char* cram)
{
    if (!vram || !cram || !ensureTitleDecodedTexture())
        return false;

    // The title NBG0 artwork is static for the lifetime of this front-end
    // resource set. Dynamic PRESS START/menu text and selectors are separate
    // VDP2/VDP1 layers, so unrelated VRAM changes must not rebuild 704x448.
    if (g_titleDecodedValid)
        return true;

    for (unsigned int y = 0; y < kTitleDecodedHeight; ++y) {
        const unsigned int patternY = y >> 4;
        const unsigned int py = y & 15u;
        const unsigned int cellY = py >> 3;
        const unsigned int inCellY = py & 7u;

        for (unsigned int x = 0; x < kTitleDecodedWidth; ++x) {
            const unsigned int planeX =
                x >= 512u ? x - 512u : x;
            const unsigned int planeBase =
                x >= 512u ? 0x10800u : 0x10000u;
            const unsigned int patternX = planeX >> 4;
            const unsigned int patternAddress =
                planeBase + (patternY * 32u + patternX) * 2u;
            const std::uint16_t patternName =
                readVdp2Be16(vram, patternAddress);

            // Title: PNB=1, CNSM=1, CHSZ=1. Each 16x16 pattern is four
            // consecutive 8x8 cells, 8bpp, with the dot value selecting
            // Azel's 256-entry TITLEE palette at CRAM 0.
            const unsigned int characterNumber =
                static_cast<unsigned int>(patternName & 0x0FFFu) * 4u;
            const unsigned int px = planeX & 15u;
            const unsigned int cellX = px >> 3;
            const unsigned int cellIndex = cellX + cellY * 2u;
            const unsigned int dotAddress =
                characterNumber * 32u +
                cellIndex * 64u +
                inCellY * 8u +
                (px & 7u);
            const unsigned int paletteEntry =
                static_cast<unsigned int>(vram[dotAddress & 0x7FFFFu]);
            const std::uint16_t color =
                readVdp2Be16(cram, paletteEntry * 2u);
            g_titleDecodedPixels[
                y * kTitleDecodedWidth + x] =
                vdp2Rgb555ToAbgr(color);
        }
    }

    g_titleDecodedValid = true;

    static bool logged = false;
    if (!logged) {
        logging::writef(
            "[VDP2Title] decoded %ux%u RGBA cache; SGX LINEAR presentation\n",
            kTitleDecodedWidth,
            kTitleDecodedHeight);
        logged = true;
    }
    return true;
}

static std::uint32_t vdp2Rgb555ToAbgr(std::uint16_t color)
{
    const std::uint32_t r =
        static_cast<std::uint32_t>(color & 0x1Fu) * 255u / 31u;
    const std::uint32_t g =
        static_cast<std::uint32_t>((color >> 5) & 0x1Fu) * 255u / 31u;
    const std::uint32_t b =
        static_cast<std::uint32_t>((color >> 10) & 0x1Fu) * 255u / 31u;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

static void drawAzelVdp2TextLayerGpu()
{
    if (!g_vdp2TextValid ||
        !g_textureVertexProgram ||
        !g_textureFragmentProgram ||
        !g_textureWvpParam ||
        !ensureVdp2UiGpuBuffers())
        return;

    constexpr int kTileSize = 8;
    constexpr int kMapColumns = 64;
    constexpr int kVisibleColumns =
        static_cast<int>(kVdp2TextLayerWidth) / kTileSize;
    constexpr int kVisibleRows =
        static_cast<int>(kVdp2TextLayerHeight) / kTileSize;
    constexpr std::size_t kTextMapOffset = 0x6000u;
    constexpr std::size_t kFontPaletteOffset = 0x0E00u;

    std::memset(
        g_vdp2TextLayerPixels,
        0,
        kVdp2TextLayerWidth * kVdp2TextLayerHeight *
            sizeof(std::uint32_t));

    unsigned int activeCells = 0u;
    for (int ty = 0; ty < kVisibleRows; ++ty) {
        for (int tx = 0; tx < kVisibleColumns; ++tx) {
            const std::size_t mapOffset =
                kTextMapOffset +
                static_cast<std::size_t>(
                    (ty * kMapColumns + tx) * 2);
            const std::uint16_t patternName =
                readVdp2Be16(g_vdp2TextVram, mapOffset);
            if (!patternName)
                continue;
            ++activeCells;

            const unsigned int palette =
                (patternName >> 12) & 0x0Fu;
            const unsigned int tile =
                patternName & 0x0FFFu;
            const std::size_t tileOffset =
                static_cast<std::size_t>(tile) * 32u;
            if (tileOffset + 32u > kVdp2TextSnapshotBytes)
                continue;

            for (int py = 0; py < kTileSize; ++py) {
                for (int px = 0; px < kTileSize; ++px) {
                    const std::uint8_t packed =
                        g_vdp2TextVram[
                            tileOffset +
                            static_cast<std::size_t>(
                                py * 4 + px / 2)];
                    const unsigned int colorIndex =
                        (px & 1)
                            ? static_cast<unsigned int>(
                                packed & 0x0Fu)
                            : static_cast<unsigned int>(
                                packed >> 4);
                    if (!colorIndex)
                        continue;

                    const std::size_t cramOffset =
                        kFontPaletteOffset +
                        static_cast<std::size_t>(
                            (palette * 16u + colorIndex) * 2u);
                    if (cramOffset + 1u >=
                        kVdp2CramSnapshotBytes)
                        continue;

                    const unsigned int sx =
                        static_cast<unsigned int>(
                            tx * kTileSize + px);
                    const unsigned int sy =
                        static_cast<unsigned int>(
                            ty * kTileSize + py);
                    g_vdp2TextLayerPixels[
                        sy * kVdp2TextLayerWidth + sx] =
                        vdp2Rgb555ToAbgr(
                            readVdp2Be16(
                                g_vdp2Cram, cramOffset));
                }
            }
        }
    }

    if (!activeCells)
        return;

    const float renderAspect =
        static_cast<float>(viewerRenderWidth()) /
        static_cast<float>(viewerRenderHeight());
    const float xExtent =
        (4.0f / 3.0f) / renderAspect;
    g_vdp2TextLayerVertices[0] =
        {-xExtent,  1.0f, 0.5f, 0.0f, 0.0f};
    g_vdp2TextLayerVertices[1] =
        { xExtent,  1.0f, 0.5f, 1.0f, 0.0f};
    g_vdp2TextLayerVertices[2] =
        { xExtent, -1.0f, 0.5f, 1.0f, 1.0f};
    g_vdp2TextLayerVertices[3] =
        {-xExtent, -1.0f, 0.5f, 0.0f, 1.0f};

    sceGxmSetVertexProgram(
        g_probeContext, g_textureVertexProgram);
    sceGxmSetFragmentProgram(
        g_probeContext, g_textureFragmentProgram);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);

    void* uniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniforms) < 0 || !uniforms)
        return;
    static const float identity[16] = {
        1.0f,0.0f,0.0f,0.0f,
        0.0f,1.0f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f,0.0f,1.0f
    };
    sceGxmSetUniformDataF(
        uniforms, g_textureWvpParam, 0, 16, identity);
    sceGxmSetVertexStream(
        g_probeContext, 0, g_vdp2TextLayerVertices);
    sceGxmSetFragmentTexture(
        g_probeContext, 0, &g_vdp2TextLayerTexture);
    sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_vdp2TextLayerIndices,
        6);

    static bool reported = false;
    if (!reported) {
        logging::writef(
            "[VDP2Text] SGX bilinear logical layer %ux%u activeCells=%u\n",
            kVdp2TextLayerWidth,
            kVdp2TextLayerHeight,
            activeCells);
        reported = true;
    }
}


static void drawTownInputOverlay(
    std::uint32_t* buffer,
    int pitch,
    bool authenticRoomMode)
{
    if (!authenticRoomMode || !g_townPlayerReady)
        return;

    const float lx = input::analog_x();
    const float ly = input::analog_y();
    const float forward = -ly;

    const char* vertical =
        forward > 0.15f ? "FWD" :
        (forward < -0.15f ? "BACK" : "----");
    const char* horizontal =
        lx > 0.15f ? "RIGHT" :
        (lx < -0.15f ? "LEFT" : "-----");

    char line0[80];
    char line1[80];
    char line2[80];
    char line3[80];

    // Vita's compact printf path does not reliably include floating-point
    // formatting. Keep the capture overlay integer-only so recordings always
    // show usable telemetry.
    const int lx1000 =
        static_cast<int>(std::lround(lx * 1000.0f));
    const int ly1000 =
        static_cast<int>(std::lround(ly * 1000.0f));
    const int fwd1000 =
        static_cast<int>(std::lround(forward * 1000.0f));
    const int edgeX1000 =
        static_cast<int>(
            std::lround(g_townPlayerPosition[0] * 1000.0f));
    const int edgeZ1000 =
        static_cast<int>(
            std::lround(g_townPlayerPosition[2] * 1000.0f));
    const int yawDeg =
        static_cast<int>(
            std::lround(
                g_townPlayerYaw *
                (180.0f / 3.14159265358979323846f)));

    std::snprintf(
        line0, sizeof(line0),
        "STICK LX=%+04d LY=%+04d",
        lx1000,
        ly1000);
    std::snprintf(
        line1, sizeof(line1),
        "INPUT %s %s FWD=%+04d",
        vertical,
        horizontal,
        fwd1000);
    std::snprintf(
        line2, sizeof(line2),
        "EDGE X=%+05d Z=%+05d YAW=%+04d",
        edgeX1000,
        edgeZ1000,
        yawDeg);
    std::snprintf(
        line3, sizeof(line3),
        "COLL %s CONTACTS=%u Y=%+05d",
        g_townPlayerGrounded ? "GROUND" : "AIR",
        g_townCollisionContacts,
        static_cast<int>(
            std::lround(g_townPlayerPosition[1] * 1000.0f)));

    constexpr int x = 8;
    constexpr int y0 = 20;
    constexpr int lineStep = 9;

    auto shadowed = [buffer, pitch](int x, int y, const char* text) {
        drawTextSmallToBuffer(
            buffer, pitch, x + 1, y + 1,
            text, 0xFF000000u);
        drawTextSmallToBuffer(
            buffer, pitch, x, y,
            text, 0xFFFFFFFFu);
    };

    shadowed(x, y0, line0);
    shadowed(x, y0 + lineStep, line1);
    shadowed(x, y0 + lineStep * 2, line2);
    shadowed(x, y0 + lineStep * 3, line3);
}


void set_fov(float degrees)
{
    if (degrees > 1.0f && degrees < 179.0f)
        g_azelProjectionFovDegrees = degrees;
}

void invalidate_cram_range(unsigned int, unsigned int)
{
    // Neptune decodes Saturn palette/material state into its own resident
    // resources. Force the live-town material/model caches to be rebuilt on
    // the next published frame when Azel writes CRAM.
    g_liveTownMaterialCache.clear();
    g_liveTownSignature = 0;
    g_liveTownStaticSignature = 0;
    g_liveTownPrepared = false;
    g_vdp1TextureDataDirty = true;
}

void invalidate_vdp1_texture_range(unsigned int, unsigned int)
{
    // Azel's desktop backend invalidates decoded VDP1 textures here. Neptune
    // owns the Vita texture cache, so invalidate the resident live-town model
    // and material bindings and rebuild them from the updated VDP1 data.
    g_liveTownMaterialCache.clear();
    g_liveTownSignature = 0;
    g_liveTownStaticSignature = 0;
    g_liveTownPrepared = false;
    freeVdp1Textures();
    g_vdp1TextureDataDirty = true;
}

bool init()
{
    const std::size_t allocSize = (kFrameBytes + 0x3FFFFu) & ~0x3FFFFu;

    for (int i = 0; i < 2; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "LagiFramebuffer%d", i);
        g_frameMem[i] = sceKernelAllocMemBlock(
            name, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, allocSize, nullptr);
        if (g_frameMem[i] < 0) {
            shutdown();
            return false;
        }

        void* base = nullptr;
        if (sceKernelGetMemBlockBase(g_frameMem[i], &base) < 0 || !base) {
            shutdown();
            return false;
        }
        g_frameBuffer[i] = static_cast<std::uint32_t*>(base);
    }

    for (int i = 0; i < 2; ++i) {
        g_drawBuffer = i;
        fill(0xFF000000u);
        static constexpr const char* kLoadingText = "Loading...";
        static constexpr int kLoadingAdvance = 6;
        static constexpr int kLoadingMarginX = 16;
        static constexpr int kLoadingMarginY = 14;
        const int loadingLength =
            static_cast<int>(std::strlen(kLoadingText));
        drawTextSmall(
            kWidth - kLoadingMarginX - loadingLength * kLoadingAdvance,
            kHeight - kLoadingMarginY,
            kLoadingText,
            0xFFFFFFFFu);
    }
    g_drawBuffer = 0;

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = g_frameBuffer[0];
    fb.pitch = kPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

    // Match VitaSDK's established display path: queue the initial
    // framebuffer for the next scanout, wait until it becomes front, then
    // render into the other buffer.
    const int setResult = sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (setResult < 0)
        return false;

    sceDisplayWaitVblankStart();
    g_drawBuffer = 1;

    g_renderFrameReadySema =
        sceKernelCreateSema("LagiRenderReady", 0, 0, 1, nullptr);
    g_renderFrameFreeSema =
        sceKernelCreateSema("LagiRenderFree", 0, 1, 1, nullptr);
    g_movieFrameSema =
        sceKernelCreateSema("LagiMovieFrame", 0, 1, 1, nullptr);
    if (g_renderFrameReadySema < 0 || g_renderFrameFreeSema < 0 ||
        g_movieFrameSema < 0) {
        shutdown();
        return false;
    }

    g_renderThreadRunning = true;
    g_renderThread = sceKernelCreateThread(
        "LagiRender",
        renderThreadMain,
        0x10000100,
        0x20000,
        0,
        0,
        nullptr);
    if (g_renderThread < 0) {
        g_renderThreadRunning = false;
        shutdown();
        return false;
    }

    if (sceKernelStartThread(g_renderThread, 0, nullptr) < 0) {
        g_renderThreadRunning = false;
        sceKernelDeleteThread(g_renderThread);
        g_renderThread = -1;
        shutdown();
        return false;
    }
    g_renderThreadStarted = true;
    status("[PASS] DEDICATED RENDER THREAD", 0xFF80E0FFu);
    return true;
}

void shutdown()
{
    // Stop renderer ownership before releasing any GXM/context resources.
    if (g_renderThreadStarted && g_renderThread >= 0) {
        g_renderThreadRunning = false;
        if (g_renderFrameReadySema >= 0)
            sceKernelSignalSema(g_renderFrameReadySema, 1);
        int threadStatus = 0;
        sceKernelWaitThreadEnd(g_renderThread, &threadStatus, nullptr);
        sceKernelDeleteThread(g_renderThread);
        g_renderThread = -1;
        g_renderThreadStarted = false;
    }
    if (g_renderFrameReadySema >= 0) {
        sceKernelDeleteSema(g_renderFrameReadySema);
        g_renderFrameReadySema = -1;
    }
    if (g_renderFrameFreeSema >= 0) {
        sceKernelDeleteSema(g_renderFrameFreeSema);
        g_renderFrameFreeSema = -1;
    }
    if (g_movieFrameSema >= 0) {
        sceKernelDeleteSema(g_movieFrameSema);
        g_movieFrameSema = -1;
    }

    auto freeSimpleMappedProbe = [](SceUID& uid, void*& ptr) {
        if (uid >= 0) {
            void* mem = nullptr;
            if (sceKernelGetMemBlockBase(uid, &mem) >= 0 && mem)
                sceGxmUnmapMemory(mem);
            sceKernelFreeMemBlock(uid);
            uid = -1;
            ptr = nullptr;
        }
    };

    void* fadeVertexPtr = g_fadeVertices;
    freeSimpleMappedProbe(g_fadeVertexUid, fadeVertexPtr);
    g_fadeVertices = nullptr;
    void* fadeIndexPtr = g_fadeIndices;
    freeSimpleMappedProbe(g_fadeIndexUid, fadeIndexPtr);
    g_fadeIndices = nullptr;

    void* vdp2AtlasPtr = g_vdp2Nbg1AtlasPixels;
    freeSimpleMappedProbe(g_vdp2Nbg1AtlasUid, vdp2AtlasPtr);
    g_vdp2Nbg1AtlasPixels = nullptr;
    void* vdp2VertexPtr = g_vdp2Nbg1Vertices;
    freeSimpleMappedProbe(g_vdp2Nbg1VertexUid, vdp2VertexPtr);
    g_vdp2Nbg1Vertices = nullptr;
    void* vdp2IndexPtr = g_vdp2Nbg1Indices;
    freeSimpleMappedProbe(g_vdp2Nbg1IndexUid, vdp2IndexPtr);
    g_vdp2Nbg1Indices = nullptr;
    void* vdp2TextLayerPtr = g_vdp2TextLayerPixels;
    freeSimpleMappedProbe(g_vdp2TextLayerUid, vdp2TextLayerPtr);
    g_vdp2TextLayerPixels = nullptr;
    void* vdp2TextVertexPtr = g_vdp2TextLayerVertices;
    freeSimpleMappedProbe(g_vdp2TextLayerVertexUid, vdp2TextVertexPtr);
    g_vdp2TextLayerVertices = nullptr;
    void* vdp2TextIndexPtr = g_vdp2TextLayerIndices;
    freeSimpleMappedProbe(g_vdp2TextLayerIndexUid, vdp2TextIndexPtr);
    g_vdp2TextLayerIndices = nullptr;
    void* vdp2BarVertexPtr = g_vdp2BarVertices;
    freeSimpleMappedProbe(g_vdp2BarVertexUid, vdp2BarVertexPtr);
    g_vdp2BarVertices = nullptr;
    void* vdp2BarIndexPtr = g_vdp2BarIndices;
    freeSimpleMappedProbe(g_vdp2BarIndexUid, vdp2BarIndexPtr);
    g_vdp2BarIndices = nullptr;

    void* basicWingVertexPtr = g_vdp1Vertices;
    freeSimpleMappedProbe(g_vdp1VertexUid, basicWingVertexPtr);
    g_vdp1Vertices = nullptr;
    void* basicWingLightingPtr = g_vdp1LightingVertices;
    freeSimpleMappedProbe(g_vdp1LightingVertexUid, basicWingLightingPtr);
    g_vdp1LightingVertices = nullptr;
    void* basicWingIndexPtr = g_vdp1Indices;
    freeSimpleMappedProbe(g_vdp1IndexUid, basicWingIndexPtr);
    g_vdp1Indices = nullptr;

    void* textureVertexPtr = g_vdp1TextureVertices;
    freeSimpleMappedProbe(g_vdp1TextureVertexUid, textureVertexPtr);
    g_vdp1TextureVertices = nullptr;
    void* gouraudVertexPtr = g_vdp1GouraudVertices;
    freeSimpleMappedProbe(g_vdp1GouraudVertexUid, gouraudVertexPtr);
    g_vdp1GouraudVertices = nullptr;
    void* textureIndexPtr = g_vdp1TextureIndices;
    freeSimpleMappedProbe(g_vdp1TextureIndexUid, textureIndexPtr);
    g_vdp1TextureIndices = nullptr;
    void* subdivVertexPtr = g_vdp1SubdivVertices;
    freeSimpleMappedProbe(g_vdp1SubdivVertexUid, subdivVertexPtr);
    g_vdp1SubdivVertices = nullptr;
    void* subdivIndexPtr = g_vdp1SubdivIndices;
    freeSimpleMappedProbe(g_vdp1SubdivIndexUid, subdivIndexPtr);
    g_vdp1SubdivIndices = nullptr;
    g_vdp1SubdivVertexCapacity = 0u;
    g_vdp1SubdivIndexCapacity = 0u;
    g_vdp1SubdivQuadIndices.clear();
    freeVdp1Textures();
    freeMovieResources();

    if (g_probeShaderPatcher) {
        if (g_gouraudScanlineGrayFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_gouraudScanlineGrayFragmentProgram);
            g_gouraudScanlineGrayFragmentProgram = nullptr;
        }
        if (g_texturedGouraudScanlineFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedGouraudScanlineFragmentProgram);
            g_texturedGouraudScanlineFragmentProgram = nullptr;
        }
        if (g_texturedGouraudFinalQuantFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedGouraudFinalQuantFragmentProgram);
            g_texturedGouraudFinalQuantFragmentProgram = nullptr;
        }
        if (g_texturedRgb555NoGouraudFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedRgb555NoGouraudFragmentProgram);
            g_texturedRgb555NoGouraudFragmentProgram = nullptr;
        }
        if (g_texturedGouraudNoQuantFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedGouraudNoQuantFragmentProgram);
            g_texturedGouraudNoQuantFragmentProgram = nullptr;
        }
        if (g_texturedGouraudNoInverseFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedGouraudNoInverseFragmentProgram);
            g_texturedGouraudNoInverseFragmentProgram = nullptr;
        }
        if (g_texturedPayloadProbeFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedPayloadProbeFragmentProgram);
            g_texturedPayloadProbeFragmentProgram = nullptr;
        }
        if (g_texturedLitNewtonFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedLitNewtonFragmentProgram);
            g_texturedLitNewtonFragmentProgram = nullptr;
        }
        if (g_texturedLitHalfFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedLitHalfFragmentProgram);
            g_texturedLitHalfFragmentProgram = nullptr;
        }
        if (g_texturedLitOpaqueFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedLitOpaqueFragmentProgram);
            g_texturedLitOpaqueFragmentProgram = nullptr;
        }
        if (g_texturedLitFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedLitFragmentProgram);
            g_texturedLitFragmentProgram = nullptr;
        }
        if (g_gouraudDebugFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_gouraudDebugFragmentProgram);
            g_gouraudDebugFragmentProgram = nullptr;
        }
        if (g_meshSubdivFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_meshSubdivFragmentProgram);
            g_meshSubdivFragmentProgram = nullptr;
        }
        if (g_meshTextureFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_meshTextureFragmentProgram);
            g_meshTextureFragmentProgram = nullptr;
        }
        if (g_vdp2Rbg0FragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_vdp2Rbg0FragmentProgram);
            g_vdp2Rbg0FragmentProgram = nullptr;
        }
        if (g_vdp2NbgFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_vdp2NbgFragmentProgram);
            g_vdp2NbgFragmentProgram = nullptr;
        }
        if (g_cinepakFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_cinepakFragmentProgram);
            g_cinepakFragmentProgram = nullptr;
        }
        if (g_movieTextureFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_movieTextureFragmentProgram);
            g_movieTextureFragmentProgram = nullptr;
        }
        if (g_textureFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_textureFragmentProgram);
            g_textureFragmentProgram = nullptr;
        }
        if (g_gouraudSubdivGrayFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_gouraudSubdivGrayFragmentProgram);
            g_gouraudSubdivGrayFragmentProgram = nullptr;
        }
        if (g_texturedGouraudSubdivFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_texturedGouraudSubdivFragmentProgram);
            g_texturedGouraudSubdivFragmentProgram = nullptr;
        }
        if (g_gouraudSubdivVertexProgram) {
            sceGxmShaderPatcherReleaseVertexProgram(
                g_probeShaderPatcher, g_gouraudSubdivVertexProgram);
            g_gouraudSubdivVertexProgram = nullptr;
        }
        if (g_gouraudPayloadVertexProgram) {
            sceGxmShaderPatcherReleaseVertexProgram(
                g_probeShaderPatcher, g_gouraudPayloadVertexProgram);
            g_gouraudPayloadVertexProgram = nullptr;
        }
        if (g_textureVertexProgram) {
            sceGxmShaderPatcherReleaseVertexProgram(
                g_probeShaderPatcher, g_textureVertexProgram);
            g_textureVertexProgram = nullptr;
        }
        if (g_gouraudScanlineGrayFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_gouraudScanlineGrayFragmentProgramId);
            g_gouraudScanlineGrayFragmentRegistered = false;
        }
        if (g_texturedGouraudScanlineFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedGouraudScanlineFragmentProgramId);
            g_texturedGouraudScanlineFragmentRegistered = false;
        }
        if (g_texturedGouraudFinalQuantFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedGouraudFinalQuantFragmentProgramId);
            g_texturedGouraudFinalQuantFragmentRegistered = false;
        }
        if (g_texturedRgb555NoGouraudFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedRgb555NoGouraudFragmentProgramId);
            g_texturedRgb555NoGouraudFragmentRegistered = false;
        }
        if (g_texturedGouraudNoQuantFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedGouraudNoQuantFragmentProgramId);
            g_texturedGouraudNoQuantFragmentRegistered = false;
        }
        if (g_texturedGouraudNoInverseFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedGouraudNoInverseFragmentProgramId);
            g_texturedGouraudNoInverseFragmentRegistered = false;
        }
        if (g_texturedPayloadProbeFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedPayloadProbeFragmentProgramId);
            g_texturedPayloadProbeFragmentRegistered = false;
        }
        if (g_texturedLitNewtonFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedLitNewtonFragmentProgramId);
            g_texturedLitNewtonFragmentRegistered = false;
        }
        if (g_texturedLitHalfFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedLitHalfFragmentProgramId);
            g_texturedLitHalfFragmentRegistered = false;
        }
        if (g_texturedLitOpaqueFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedLitOpaqueFragmentProgramId);
            g_texturedLitOpaqueFragmentRegistered = false;
        }
        if (g_texturedLitFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedLitFragmentProgramId);
            g_texturedLitFragmentRegistered = false;
        }
        if (g_gouraudDebugFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_gouraudDebugFragmentProgramId);
            g_gouraudDebugFragmentRegistered = false;
        }
        if (g_meshFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_meshFragmentProgramId);
            g_meshFragmentRegistered = false;
        }
        if (g_vdp2Rbg0FragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_vdp2Rbg0FragmentProgramId);
            g_vdp2Rbg0FragmentRegistered = false;
            g_vdp2Rbg0Available = false;
        }
        if (g_vdp2NbgFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_vdp2NbgFragmentProgramId);
            g_vdp2NbgFragmentRegistered = false;
        }
        if (g_cinepakFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_cinepakFragmentProgramId);
            g_cinepakFragmentRegistered = false;
        }
        if (g_textureFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_textureFragmentProgramId);
            g_textureFragmentRegistered = false;
        }
        if (g_gouraudSubdivGrayFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_gouraudSubdivGrayFragmentProgramId);
            g_gouraudSubdivGrayFragmentRegistered = false;
        }
        if (g_texturedGouraudSubdivFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_texturedGouraudSubdivFragmentProgramId);
            g_texturedGouraudSubdivFragmentRegistered = false;
        }
        if (g_gouraudSubdivVertexRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_gouraudSubdivVertexProgramId);
            g_gouraudSubdivVertexRegistered = false;
        }
        if (g_gouraudPayloadVertexRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_gouraudPayloadVertexProgramId);
            g_gouraudPayloadVertexRegistered = false;
        }
        if (g_textureVertexRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_textureVertexProgramId);
            g_textureVertexRegistered = false;
        }

        if (g_colorOffsetAddFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_colorOffsetAddFragmentProgram);
            g_colorOffsetAddFragmentProgram = nullptr;
        }
        if (g_colorOffsetSubtractFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_colorOffsetSubtractFragmentProgram);
            g_colorOffsetSubtractFragmentProgram = nullptr;
        }
        if (g_fadeFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_fadeFragmentProgram);
            g_fadeFragmentProgram = nullptr;
        }
        if (g_probeFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_probeFragmentProgram);
            g_probeFragmentProgram = nullptr;
        }
        if (g_probeVertexProgram) {
            sceGxmShaderPatcherReleaseVertexProgram(
                g_probeShaderPatcher, g_probeVertexProgram);
            g_probeVertexProgram = nullptr;
        }

        if (g_probeFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_probeFragmentProgramId);
            g_probeFragmentRegistered = false;
        }
        if (g_probeVertexRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_probeVertexProgramId);
            g_probeVertexRegistered = false;
        }
        sceGxmShaderPatcherDestroy(g_probeShaderPatcher);
        g_probeShaderPatcher = nullptr;
    }

    if (g_probePatcherVertexUsseUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_probePatcherVertexUsseUid, &mem) >= 0 && mem)
            sceGxmUnmapVertexUsseMemory(mem);
        sceKernelFreeMemBlock(g_probePatcherVertexUsseUid);
        g_probePatcherVertexUsseUid = -1;
        g_probePatcherVertexUsse = nullptr;
    }

    if (g_probePatcherFragmentUsseUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_probePatcherFragmentUsseUid, &mem) >= 0 && mem)
            sceGxmUnmapFragmentUsseMemory(mem);
        sceKernelFreeMemBlock(g_probePatcherFragmentUsseUid);
        g_probePatcherFragmentUsseUid = -1;
        g_probePatcherFragmentUsse = nullptr;
    }

    if (g_probePatcherBufferUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_probePatcherBufferUid, &mem) >= 0 && mem)
            sceGxmUnmapMemory(mem);
        sceKernelFreeMemBlock(g_probePatcherBufferUid);
        g_probePatcherBufferUid = -1;
        g_probePatcherBuffer = nullptr;
    }

    if (g_probeSync2) {
        sceGxmSyncObjectDestroy(g_probeSync2);
        g_probeSync2 = nullptr;
    }
    if (g_probeSync) {
        sceGxmSyncObjectDestroy(g_probeSync);
        g_probeSync = nullptr;
    }

    auto freeProbeMapped = [](SceUID& uid, void*& ptr) {
        if (uid >= 0) {
            void* mem = nullptr;
            if (sceKernelGetMemBlockBase(uid, &mem) >= 0 && mem)
                sceGxmUnmapMemory(mem);
            sceKernelFreeMemBlock(uid);
            uid = -1;
            ptr = nullptr;
        }
    };

    void* cinepakResolvePtr = g_cinepakResolveColorBuffer;
    freeProbeMapped(g_cinepakResolveColorUid, cinepakResolvePtr);
    g_cinepakResolveColorBuffer = nullptr;
    g_cinepakResolveColorSurface = {};
    g_cinepakResolveTexture = {};

    void* colorPtr2 = g_probeColorBuffer2;
    freeProbeMapped(g_probeColorUid2, colorPtr2);
    g_probeColorBuffer2 = nullptr;

    void* colorPtr = g_probeColorBuffer;
    freeProbeMapped(g_probeColorUid, colorPtr);
    g_probeColorBuffer = nullptr;
    freeProbeMapped(g_probeDepthUid, g_probeDepth);
    freeProbeMapped(g_probeStencilUid, g_probeStencil);

    if (g_frontendHighRenderTarget) {
        sceGxmDestroyRenderTarget(g_frontendHighRenderTarget);
        g_frontendHighRenderTarget = nullptr;
    }
    if (g_movieRenderTarget) {
        sceGxmDestroyRenderTarget(g_movieRenderTarget);
        g_movieRenderTarget = nullptr;
    }
    if (g_probeRenderTargetHalf) {
        sceGxmDestroyRenderTarget(g_probeRenderTargetHalf);
        g_probeRenderTargetHalf = nullptr;
    }
    if (g_probeRenderTarget) {
        sceGxmDestroyRenderTarget(g_probeRenderTarget);
        g_probeRenderTarget = nullptr;
    }

    if (g_probeContext) {
        sceGxmDestroyContext(g_probeContext);
        g_probeContext = nullptr;
    }
    std::free(g_probeContextHost);
    g_probeContextHost = nullptr;

    if (g_probeFragmentUsseUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_probeFragmentUsseUid, &mem) >= 0 && mem)
            sceGxmUnmapFragmentUsseMemory(mem);
        sceKernelFreeMemBlock(g_probeFragmentUsseUid);
        g_probeFragmentUsseUid = -1;
        g_probeFragmentUsse = nullptr;
    }

    struct ProbeMappedBlock { SceUID* uid; void** ptr; };
    ProbeMappedBlock mapped[] = {
        { &g_probeVdmUid, &g_probeVdm },
        { &g_probeVertexUid, &g_probeVertex },
        { &g_probeFragmentUid, &g_probeFragment }
    };
    for (auto& block : mapped) {
        if (*block.uid >= 0) {
            void* mem = nullptr;
            if (sceKernelGetMemBlockBase(*block.uid, &mem) >= 0 && mem)
                sceGxmUnmapMemory(mem);
            sceKernelFreeMemBlock(*block.uid);
            *block.uid = -1;
            *block.ptr = nullptr;
        }
    }

    if (g_gxmInitialized) {
        sceGxmTerminate();
        g_gxmInitialized = false;
    }

    for (int i = 0; i < 2; ++i) {
        if (g_frameMem[i] >= 0) {
            sceKernelFreeMemBlock(g_frameMem[i]);
            g_frameMem[i] = -1;
        }
        g_frameBuffer[i] = nullptr;
    }
    g_drawBuffer = 0;
    g_viewerReady = false;
    g_probeDisplayingGxm = false;
    g_gxmDrawBuffer = 1;
}

void status(const char* text, unsigned int color)
{
    if (!text || g_statusCount >= kMaxStatus) return;

    if (g_suppressGxmInitPassStatus &&
        std::strncmp(text, "[PASS] GXM ", 11) == 0)
        return;

    StatusLine& line = g_status[g_statusCount++];
    std::strncpy(line.text, text, sizeof(line.text) - 1);
    line.text[sizeof(line.text) - 1] = 0;
    line.color = color;
}

void failure(const char* text)
{
    // If GXM initialization fails, stop suppressing its status stream so the
    // failure remains visible and future diagnostics are not hidden.
    g_suppressGxmInitPassStatus = false;
    status(text, 0xFF3030FFu);
}

void set_azel_alive(bool alive) { g_azelAlive = alive; }
void set_disc_alive(bool alive) { g_discAlive = alive; }

static unsigned int probeAlign4096(unsigned int size)
{
    return (size + 4095u) & ~4095u;
}

static void* probeGpuAlloc(unsigned int size, unsigned int attribs, SceUID* uid)
{
    const unsigned int bytes = probeAlign4096(size);
    *uid = sceKernelAllocMemBlock(
        "LagiGxmProbe", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        bytes, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    if (sceGxmMapMemory(
            mem, bytes,
            static_cast<SceGxmMemoryAttribFlags>(attribs)) < 0) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    return mem;
}

static void* probeCdramAlloc(unsigned int size, unsigned int attribs, SceUID* uid)
{
    const unsigned int bytes =
        (size + (256u * 1024u - 1u)) & ~(256u * 1024u - 1u);
    *uid = sceKernelAllocMemBlock(
        "LagiGxmProbeCDRAM", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
        bytes, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    if (sceGxmMapMemory(mem, bytes,
            static_cast<SceGxmMemoryAttribFlags>(attribs)) < 0) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }
    return mem;
}

static void freeMovieMappedBlock(SceUID& uid, void*& memory)
{
    if (uid >= 0) {
        if (memory)
            sceGxmUnmapMemory(memory);
        sceKernelFreeMemBlock(uid);
    }
    uid = -1;
    memory = nullptr;
}

static void freeMovieResources()
{
    freeMovieMappedBlock(g_movieTextureUid, g_movieTextureData);

    void* titlePixels = g_titleDecodedPixels;
    freeMovieMappedBlock(g_titleDecodedUid, titlePixels);
    g_titleDecodedPixels = nullptr;
    g_titleDecodedTexture = {};
    g_titleDecodedValid = false;

    void* vertices = g_movieVertices;
    freeMovieMappedBlock(g_movieVertexUid, vertices);
    g_movieVertices = nullptr;

    void* indices = g_movieIndices;
    freeMovieMappedBlock(g_movieIndexUid, indices);
    g_movieIndices = nullptr;

    void* windowVertices = g_vdp2WindowVertices;
    freeMovieMappedBlock(g_vdp2WindowVertexUid, windowVertices);
    g_vdp2WindowVertices = nullptr;

    void* windowIndices = g_vdp2WindowIndices;
    freeMovieMappedBlock(g_vdp2WindowIndexUid, windowIndices);
    g_vdp2WindowIndices = nullptr;

    void* resolveVertices = g_cinepakResolveVertices;
    freeMovieMappedBlock(g_cinepakResolveVertexUid, resolveVertices);
    g_cinepakResolveVertices = nullptr;

    void* resolveIndices = g_cinepakResolveIndices;
    freeMovieMappedBlock(g_cinepakResolveIndexUid, resolveIndices);
    g_cinepakResolveIndices = nullptr;

    g_movieTexture = {};
    g_movieWidth = 0;
    g_movieHeight = 0;
    g_movieStridePixels = 0;
    g_moviePayloadWidth = 0;
    g_moviePayloadHeight = 0;
    g_movieUsesCinepakPayload = false;
    g_movieUsesVdp2Title = false;
    g_movieFrameVisible = false;
    g_movieUploadLogged = false;
    g_movieRenderLogged = false;
}

bool movie_present_frame(
    const std::uint32_t* rgba,
    unsigned int width,
    unsigned int height,
    unsigned int pitchPixels)
{
    if (!g_gxmInitialized || !g_probeContext || !rgba ||
        !width || !height || pitchPixels < width)
        return false;

    // Scene presentation normally acquires this producer token in
    // presentation_wait_frame_slot(). Movie playback bypasses the scene presentation publish path,
    // so it must participate in the same one-frame ownership protocol itself.
    // Without this handoff a movie upload can race the dedicated render thread
    // or leave a ready notification disconnected from renderer ownership.
    MovieRenderSlotGuard renderSlot;
    if (!renderSlot)
        return false;

    MovieFrameGuard guard;
    if (!guard)
        return false;

    const unsigned int stridePixels = (width + 7u) & ~7u;
    if (!g_movieTextureData || width != g_movieWidth ||
        height != g_movieHeight || stridePixels != g_movieStridePixels) {
        freeMovieResources();

        const unsigned int textureBytes =
            stridePixels * height * sizeof(std::uint32_t);
        g_movieTextureData = probeGpuAlloc(
            textureBytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_movieTextureUid);
        g_movieVertices = static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                4u * sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_movieVertexUid));
        g_movieIndices = static_cast<std::uint16_t*>(
            probeGpuAlloc(
                6u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_movieIndexUid));

        // A Saturn line window can expose up to two horizontal spans on each
        // of 224 scanlines (outside-window mode). Build that coverage as
        // reusable textured geometry instead of branching/reading VRAM inside
        // the RBG0 fragment shader.
        constexpr unsigned int kWindowMaxQuads = 224u * 2u;
        g_vdp2WindowVertices = static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                kWindowMaxQuads * 4u * sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2WindowVertexUid));
        g_vdp2WindowIndices = static_cast<std::uint16_t*>(
            probeGpuAlloc(
                kWindowMaxQuads * 6u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2WindowIndexUid));

        if (!g_movieTextureData || !g_movieVertices || !g_movieIndices ||
            !g_vdp2WindowVertices || !g_vdp2WindowIndices) {
            freeMovieResources();
            return false;
        }

        if (sceGxmTextureInitLinear(
                &g_movieTexture,
                g_movieTextureData,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
                width,
                height,
                0) < 0) {
            freeMovieResources();
            return false;
        }
        sceGxmTextureSetMinFilter(
            &g_movieTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);
        sceGxmTextureSetMagFilter(
            &g_movieTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);

        // Full-screen movie presentation preserves the source aspect while
        // filling the Vita output vertically. Wider-than-display movies are
        // clipped symmetrically at the left/right edges rather than distorted.
        const float displayAspect =
            static_cast<float>(viewerRenderWidth()) /
            static_cast<float>(viewerRenderHeight());
        const float sourceAspect =
            static_cast<float>(width) /
            static_cast<float>(height);
        const float xExtent = sourceAspect / displayAspect;
        const float yExtent = 1.0f;
        g_movieVertices[0] = {-xExtent,  yExtent, 0.5f, 0.0f, 0.0f};
        g_movieVertices[1] = { xExtent,  yExtent, 0.5f, 1.0f, 0.0f};
        g_movieVertices[2] = {-xExtent, -yExtent, 0.5f, 0.0f, 1.0f};
        g_movieVertices[3] = { xExtent, -yExtent, 0.5f, 1.0f, 1.0f};
        const std::uint16_t indices[6] = {0, 1, 2, 2, 1, 3};
        std::memcpy(g_movieIndices, indices, sizeof(indices));

        g_movieWidth = width;
        g_movieHeight = height;
        g_movieStridePixels = stridePixels;
    }

    g_movieUsesCinepakPayload = false;
    g_movieUsesVdp2Title = false;
    g_moviePayloadWidth = 0;
    g_moviePayloadHeight = 0;

    auto* destination = static_cast<std::uint32_t*>(g_movieTextureData);
    for (unsigned int y = 0; y < height; ++y) {
        std::memcpy(
            destination + y * g_movieStridePixels,
            rgba + y * pitchPixels,
            width * sizeof(std::uint32_t));
    }
    g_movieFrameVisible = true;
    if (!g_movieUploadLogged) {
        logging::writef(
            "[MovieRender] first upload %ux%u pitch=%u textureStride=%u\n",
            width, height, pitchPixels, g_movieStridePixels);
        g_movieUploadLogged = true;
    }

    if (!renderSlot.publish()) {
        logging::writef("[MovieRender] FAIL publish render slot\n");
        return false;
    }
    return true;
}

bool movie_present_cinepak_payload(
    const std::uint32_t* payload,
    unsigned int payloadWidth,
    unsigned int payloadHeight,
    unsigned int sourceWidth,
    unsigned int sourceHeight)
{
    if (!g_gxmInitialized || !g_probeContext || !payload ||
        !payloadWidth || !payloadHeight || !sourceWidth || !sourceHeight)
        return false;

    MovieRenderSlotGuard renderSlot;
    if (!renderSlot)
        return false;

    MovieFrameGuard guard;
    if (!guard)
        return false;

    // The payload texture is eight RGBA8 texels per 4x4 source block.
    // payloadWidth is therefore naturally 8-texel aligned.
    if (!g_movieTextureData ||
        sourceWidth != g_movieWidth ||
        sourceHeight != g_movieHeight ||
        payloadWidth != g_moviePayloadWidth ||
        payloadHeight != g_moviePayloadHeight ||
        !g_movieUsesCinepakPayload) {
        freeMovieResources();

        const unsigned int textureBytes =
            payloadWidth * payloadHeight * sizeof(std::uint32_t);
        g_movieTextureData = probeGpuAlloc(
            textureBytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_movieTextureUid);
        g_movieVertices = static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                4u * sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_movieVertexUid));
        g_movieIndices = static_cast<std::uint16_t*>(
            probeGpuAlloc(
                6u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_movieIndexUid));
        g_cinepakResolveVertices = static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                4u * sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_cinepakResolveVertexUid));
        g_cinepakResolveIndices = static_cast<std::uint16_t*>(
            probeGpuAlloc(
                6u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_cinepakResolveIndexUid));
        if (!g_movieTextureData || !g_movieVertices || !g_movieIndices ||
            !g_cinepakResolveVertices || !g_cinepakResolveIndices) {
            freeMovieResources();
            return false;
        }

        if (sceGxmTextureInitLinear(
                &g_movieTexture,
                g_movieTextureData,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
                payloadWidth,
                payloadHeight,
                0) < 0) {
            freeMovieResources();
            return false;
        }
        sceGxmTextureSetMinFilter(
            &g_movieTexture, SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(
            &g_movieTexture, SCE_GXM_TEXTURE_FILTER_POINT);

        if (sourceWidth > static_cast<unsigned int>(kWidth / 2) ||
            sourceHeight > static_cast<unsigned int>(kHeight / 2)) {
            logging::writef(
                "[MovieRender] Cinepak source %ux%u exceeds resolve surface\n",
                sourceWidth, sourceHeight);
            freeMovieResources();
            return false;
        }
        if (sceGxmTextureInitLinearStrided(
                &g_cinepakResolveTexture,
                g_cinepakResolveColorBuffer,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
                sourceWidth,
                sourceHeight,
                512u * sizeof(std::uint32_t)) < 0) {
            freeMovieResources();
            return false;
        }
        sceGxmTextureSetMinFilter(
            &g_cinepakResolveTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);
        sceGxmTextureSetMagFilter(
            &g_cinepakResolveTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);

        g_cinepakResolveVertices[0] = {-1.0f,  1.0f, 0.5f, 0.0f, 0.0f};
        g_cinepakResolveVertices[1] = { 1.0f,  1.0f, 0.5f, 1.0f, 0.0f};
        g_cinepakResolveVertices[2] = {-1.0f, -1.0f, 0.5f, 0.0f, 1.0f};
        g_cinepakResolveVertices[3] = { 1.0f, -1.0f, 0.5f, 1.0f, 1.0f};
        const std::uint16_t resolveIndices[6] = {0, 1, 2, 2, 1, 3};
        std::memcpy(
            g_cinepakResolveIndices, resolveIndices, sizeof(resolveIndices));

        const float displayAspect =
            static_cast<float>(viewerRenderWidth()) /
            static_cast<float>(viewerRenderHeight());
        const float sourceAspect =
            static_cast<float>(sourceWidth) /
            static_cast<float>(sourceHeight);

        // Cinepak movies use the same horizontal presentation width as the
        // normal Saturn 4:3 image on Vita. Preserve the movie's own aspect
        // ratio inside that width, leaving black above/below for widescreen
        // sources instead of filling vertically and cropping the sides.
        const float xExtent = (4.0f / 3.0f) / displayAspect;
        const float yExtent =
            xExtent * displayAspect / sourceAspect;
        g_movieVertices[0] = {-xExtent,  yExtent, 0.5f, 0.0f, 0.0f};
        g_movieVertices[1] = { xExtent,  yExtent, 0.5f, 1.0f, 0.0f};
        g_movieVertices[2] = {-xExtent, -yExtent, 0.5f, 0.0f, 1.0f};
        g_movieVertices[3] = { xExtent, -yExtent, 0.5f, 1.0f, 1.0f};
        const std::uint16_t indices[6] = {0, 1, 2, 2, 1, 3};
        std::memcpy(g_movieIndices, indices, sizeof(indices));

        g_movieWidth = sourceWidth;
        g_movieHeight = sourceHeight;
        g_movieStridePixels = 0;
        g_moviePayloadWidth = payloadWidth;
        g_moviePayloadHeight = payloadHeight;
        g_movieUsesCinepakPayload = true;
        g_movieUsesVdp2Title = false;
    }

    std::memcpy(
        g_movieTextureData,
        payload,
        static_cast<std::size_t>(payloadWidth) *
            payloadHeight * sizeof(std::uint32_t));

    g_movieFrameVisible = true;
    if (!g_movieUploadLogged) {
        logging::writef(
            "[MovieRender] SGX Cinepak payload %ux%u for %ux%u source\n",
            payloadWidth, payloadHeight, sourceWidth, sourceHeight);
        g_movieUploadLogged = true;
    }

    if (!renderSlot.publish()) {
        logging::writef("[MovieRender] FAIL publish SGX Cinepak slot\n");
        return false;
    }
    return true;
}

bool movie_republish_frame()
{
    MovieRenderSlotGuard renderSlot;
    if (!renderSlot)
        return false;

    MovieFrameGuard guard;
    if (!guard || !g_movieFrameVisible || !g_movieTextureData)
        return false;

    return renderSlot.publish();
}

void frontend_set_rbg0_state(const FrontendRbg0State& state)
{
    for (unsigned int i = 0; i < 16u; ++i) {
        g_movieRbg0Planes[i] =
            static_cast<float>(state.planeA[i]);
        g_movieRbg0PlanesB[i] =
            static_cast<float>(state.planeB[i]);
    }

    // Four float4 uniforms carry only renderer-facing VDP2 register state.
    // Values are all <= 19 bits (or 16-bit registers), exactly representable
    // as floats on SGX. The shader continues to fetch tile/parameter/
    // coefficient/window data from the raw VRAM snapshot.
    g_movieRbg0Ctrl[0] = static_cast<float>(state.rpmd);
    g_movieRbg0Plsz = static_cast<float>(state.plsz);
    g_movieRbg0Format[0] = static_cast<float>(state.chctlb);
    g_movieRbg0Format[1] = static_cast<float>(state.pncr);
    g_movieRbg0Format[2] = static_cast<float>(state.craofb);
    g_movieRbg0Format[3] = static_cast<float>(state.plsz);
    g_movieRbg0Ctrl[1] = static_cast<float>(state.ktctl);
    g_movieRbg0Ctrl[2] = static_cast<float>(state.ktaof);
    g_movieRbg0Ctrl[3] = static_cast<float>(state.wctlc);

    g_movieRbg0Ctrl[4] = static_cast<float>(state.wctld);
    g_movieRbg0Ctrl[5] =
        static_cast<float>(state.lineWindow0Address);
    g_movieRbg0Ctrl[6] =
        static_cast<float>(state.lineWindow1Address);
    g_movieRbg0Ctrl[7] =
        static_cast<float>(state.lineWindowMask);

    for (unsigned int i = 0; i < 4u; ++i) {
        g_movieRbg0Ctrl[8u + i] =
            static_cast<float>(state.window0[i]);
        g_movieRbg0Ctrl[12u + i] =
            static_cast<float>(state.window1[i]);
        g_movieRbg0CoefficientA[i] = state.coefficientA[i];
        g_movieRbg0CoefficientB[i] = state.coefficientB[i];
    }
    for (unsigned int i = 0; i < 8u; ++i) {
        g_movieRbg0TransformA[i] = state.transformA[i];
        g_movieRbg0TransformB[i] = state.transformB[i];
    }
}

bool frontend_present_vdp2(
    const unsigned char* vram,
    const unsigned char* cram,
    unsigned int layout,
    int scrollX,
    int scrollY,
    unsigned int flags,
    unsigned int tvmd)
{
    if (!g_gxmInitialized || !g_probeContext || !vram || !cram)
        return false;

    MovieRenderSlotGuard renderSlot;
    if (!renderSlot)
        return false;

    // The producer owns the render slot here, so it is safe to replace the
    // published VDP1 front-end snapshot without racing the render thread.
    azel_bridge::publish_frame();

    MovieFrameGuard guard;
    if (!guard)
        return false;

    constexpr unsigned int rawWidth = 512u;
    constexpr unsigned int rawHeight = 258u;
    constexpr unsigned int rawBytes =
        rawWidth * rawHeight * sizeof(std::uint32_t);
    constexpr unsigned int vramBytes = 0x80000u;
    constexpr unsigned int cramBytes = 0x1000u;

    if (!g_movieTextureData ||
        !g_movieUsesVdp2Title ||
        g_moviePayloadWidth != rawWidth ||
        g_moviePayloadHeight != rawHeight) {
        freeMovieResources();

        g_movieTextureData = probeGpuAlloc(
            rawBytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_movieTextureUid);
        g_movieVertices = static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                4u * sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_movieVertexUid));
        g_movieIndices = static_cast<std::uint16_t*>(
            probeGpuAlloc(
                6u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_movieIndexUid));
        if (!g_movieTextureData || !g_movieVertices || !g_movieIndices) {
            freeMovieResources();
            return false;
        }

        if (sceGxmTextureInitLinear(
                &g_movieTexture,
                g_movieTextureData,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
                rawWidth,
                rawHeight,
                0) < 0) {
            freeMovieResources();
            return false;
        }
        sceGxmTextureSetMinFilter(
            &g_movieTexture, SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(
            &g_movieTexture, SCE_GXM_TEXTURE_FILTER_POINT);

        // Saturn full-screen UI is displayed at 4:3 and fills the Vita
        // vertically. The movie render target is cleared to opaque black,
        // providing the pillar bars outside this quad.
        const float displayAspect =
            static_cast<float>(viewerRenderWidth()) /
            static_cast<float>(viewerRenderHeight());
        const float xExtent =
            (4.0f / 3.0f) / displayAspect;
        const float yExtent = 1.0f;
        g_movieVertices[0] = {-xExtent,  yExtent, 0.5f, 0.0f, 0.0f};
        g_movieVertices[1] = { xExtent,  yExtent, 0.5f, 1.0f, 0.0f};
        g_movieVertices[2] = {-xExtent, -yExtent, 0.5f, 0.0f, 1.0f};
        g_movieVertices[3] = { xExtent, -yExtent, 0.5f, 1.0f, 1.0f};
        const std::uint16_t indices[6] = {0, 1, 2, 2, 1, 3};
        std::memcpy(g_movieIndices, indices, sizeof(indices));

        g_movieWidth = 352u;
        g_movieHeight = 224u;
        g_movieStridePixels = 0u;
        g_moviePayloadWidth = rawWidth;
        g_moviePayloadHeight = rawHeight;
        g_movieUsesCinepakPayload = false;
        g_movieUsesVdp2Title = true;
        g_movieUploadLogged = false;
        g_movieRenderLogged = false;
    }

    auto* raw = static_cast<unsigned char*>(g_movieTextureData);
    std::memcpy(raw, vram, vramBytes);
    std::memcpy(raw + vramBytes, cram, cramBytes);

    // VDP1 sprite color-bank decoding shares the same live VDP2 CRAM as the
    // front-end layers. Keep Neptune's CPU-side CRAM snapshot coherent with
    // the raw SGX upload so native D5 cursors/particles resolve their palette.
    static_assert(sizeof(g_vdp2Cram) <= cramBytes,
                  "front-end CRAM snapshot exceeds uploaded CRAM");
    static_assert(sizeof(g_vdp2TextVram) <= vramBytes,
                  "front-end text snapshot exceeds uploaded VRAM");
    std::memcpy(g_vdp2Cram, cram, sizeof(g_vdp2Cram));

    // The front-end NBG1/NBG3 font map uses the same Azel-authored VRAM/CRAM
    // representation as the in-game text path. Publish that snapshot to the
    // decoded RGBA text layer so SGX can bilinear-filter the final glyph image
    // instead of filtering packed Saturn memory.
    std::memcpy(g_vdp2TextVram, vram, sizeof(g_vdp2TextVram));
    g_vdp2TextValid = true;

    g_movieVdp2Info[0] = static_cast<float>(layout);
    g_movieVdp2Info[1] = static_cast<float>(scrollX);
    g_movieVdp2Info[2] = static_cast<float>(scrollY);
    g_movieVdp2Info[3] = static_cast<float>(flags);
    g_movieVdp2Tvmd = tvmd;

    // The title task begins publishing VDP2 state while TVMD is still in the
    // low-resolution setup mode. Do not freeze the decoded artwork at that
    // transient point: wait until Azel switches to the real HRESO=3,
    // double-density title mode, then decode the static 704x448 NBG0 once.
    const bool titleHighResReady =
        layout == 0u && (tvmd & 0x00C7u) == 0x00C3u;
    if (titleHighResReady &&
        !updateTitleDecodedTexture(vram, cram)) {
        logging::writef(
            "[VDP2Title] FAIL decoded title cache\n");
        return false;
    }

    g_movieFrameVisible = true;
    if (!g_movieUploadLogged) {
        logging::writef(
            "[VDP2FrontEnd] raw VRAM/CRAM upload %u bytes backend=SGX-VDP2 layout=%u\n",
            rawBytes, layout);
        g_movieUploadLogged = true;
    }

    if (!renderSlot.publish()) {
        logging::writef("[VDP2Title] FAIL publish render slot\n");
        return false;
    }
    return true;
}

void set_azel_color_offset_state(
    unsigned int enableMask,
    unsigned int selectMask,
    int aRed, int aGreen, int aBlue,
    int bRed, int bGreen, int bBlue)
{
    // Saturn VDP2 COAR/COAG/COAB/COBR/COBG/COBB are signed 9-bit
    // registers. Azel's host structs are wider and fade interpolation can
    // temporarily produce values outside [-256,255]. Real hardware masks the
    // write to 9 bits before interpreting the sign. Preserve that wrap here;
    // clamping (our previous behavior) destroys authentic white/black flashes.
    const auto signed9 = [](int value) -> int {
        value &= 0x1FF;
        return (value & 0x100) ? value - 0x200 : value;
    };

    g_azelColorOffsetEnable.store(enableMask, std::memory_order_relaxed);
    g_azelColorOffsetSelect.store(selectMask, std::memory_order_relaxed);
    g_azelColorOffsetARed.store(signed9(aRed), std::memory_order_relaxed);
    g_azelColorOffsetAGreen.store(signed9(aGreen), std::memory_order_relaxed);
    g_azelColorOffsetABlue.store(signed9(aBlue), std::memory_order_relaxed);
    g_azelColorOffsetBRed.store(signed9(bRed), std::memory_order_relaxed);
    g_azelColorOffsetBGreen.store(signed9(bGreen), std::memory_order_relaxed);
    g_azelColorOffsetBBlue.store(signed9(bBlue), std::memory_order_relaxed);
}

void movie_clear_frame()
{
    MovieFrameGuard guard;
    if (!guard)
        return;
    freeMovieResources();
}

static void* probeFragmentUsseAlloc(
    unsigned int size, SceUID* uid, unsigned int* offset)
{
    const unsigned int bytes = probeAlign4096(size);
    *uid = sceKernelAllocMemBlock(
        "LagiGxmProbeUSSE", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        bytes, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    if (sceGxmMapFragmentUsseMemory(mem, bytes, offset) < 0) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    return mem;
}

static void* probeVertexUsseAlloc(
    unsigned int size, SceUID* uid, unsigned int* offset)
{
    const unsigned int bytes = probeAlign4096(size);
    *uid = sceKernelAllocMemBlock(
        "LagiGxmProbeVUSSE", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        bytes, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    if (sceGxmMapVertexUsseMemory(mem, bytes, offset) < 0) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    return mem;
}

static void* probePatcherHostAlloc(void*, unsigned int size)
{
    return std::malloc(size);
}

static void probePatcherHostFree(void*, void* mem)
{
    std::free(mem);
}

static void freeVdp1Textures()
{
    for (auto& texture : g_vdp1GpuTextures) {
        if (texture.uid >= 0) {
            if (texture.data)
                sceGxmUnmapMemory(texture.data);
            sceKernelFreeMemBlock(texture.uid);
        }
        texture.uid = -1;
        texture.data = nullptr;
    }
    g_vdp1GpuTextures.clear();
    g_vdp1TextureBatches.clear();
    g_vdp1TexturedReady = false;
}

static bool ensureVdp1UiBuffers()
{
    if (g_vdp1UiVertices && g_vdp1UiIndices &&
        g_vdp1UiLineVertices && g_vdp1UiLineIndices)
        return true;

    g_vdp1UiVertices = static_cast<azel::DebugTextureVertex*>(
        probeGpuAlloc(
            128u * 4u * sizeof(azel::DebugTextureVertex),
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_vdp1UiVertexUid));
    g_vdp1UiIndices = static_cast<std::uint16_t*>(
        probeGpuAlloc(
            6u * sizeof(std::uint16_t),
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_vdp1UiIndexUid));
    g_vdp1UiLineVertices = static_cast<azel::DebugColorVertex*>(
        probeGpuAlloc(
            4u * sizeof(azel::DebugColorVertex),
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_vdp1UiLineVertexUid));
    g_vdp1UiLineIndices = static_cast<std::uint16_t*>(
        probeGpuAlloc(
            8u * sizeof(std::uint16_t),
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_vdp1UiLineIndexUid));
    if (!g_vdp1UiVertices || !g_vdp1UiIndices ||
        !g_vdp1UiLineVertices || !g_vdp1UiLineIndices)
        return false;

    static const std::uint16_t kIndices[6] = {0, 1, 2, 0, 2, 3};
    static const std::uint16_t kLineIndices[8] = {
        0,1, 1,2, 2,3, 3,0
    };
    std::memcpy(g_vdp1UiIndices, kIndices, sizeof(kIndices));
    std::memcpy(
        g_vdp1UiLineIndices, kLineIndices, sizeof(kLineIndices));
    return true;
}

static std::uint32_t readVdp2Be32(
    const std::uint8_t* bytes,
    std::size_t offset)
{
    return
        (static_cast<std::uint32_t>(bytes[offset]) << 24) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) |
        static_cast<std::uint32_t>(bytes[offset + 3]);
}

static bool ensureVdp2UiGpuBuffers()
{
    if (g_vdp2Nbg1AtlasPixels &&
        g_vdp2Nbg1Vertices &&
        g_vdp2Nbg1Indices &&
        g_vdp2TextLayerPixels &&
        g_vdp2TextLayerVertices &&
        g_vdp2TextLayerIndices &&
        g_vdp2BarVertices &&
        g_vdp2BarIndices)
        return true;

    const unsigned int atlasBytes =
        kVdp2Nbg1AtlasWidth *
        kVdp2Nbg1AtlasHeight *
        sizeof(std::uint32_t);
    g_vdp2Nbg1AtlasPixels =
        static_cast<std::uint32_t*>(
            probeGpuAlloc(
                atlasBytes,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2Nbg1AtlasUid));
    g_vdp2Nbg1Vertices =
        static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                kVdp2Nbg1MaxCells * 4u *
                    sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2Nbg1VertexUid));
    g_vdp2Nbg1Indices =
        static_cast<std::uint16_t*>(
            probeGpuAlloc(
                kVdp2Nbg1MaxCells * 6u *
                    sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2Nbg1IndexUid));
    const unsigned int textLayerBytes =
        kVdp2TextLayerWidth * kVdp2TextLayerHeight *
        sizeof(std::uint32_t);
    g_vdp2TextLayerPixels =
        static_cast<std::uint32_t*>(
            probeGpuAlloc(
                textLayerBytes,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2TextLayerUid));
    g_vdp2TextLayerVertices =
        static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                4u * sizeof(azel::DebugTextureVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2TextLayerVertexUid));
    g_vdp2TextLayerIndices =
        static_cast<std::uint16_t*>(
            probeGpuAlloc(
                6u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2TextLayerIndexUid));

    g_vdp2BarVertices =
        static_cast<azel::DebugColorVertex*>(
            probeGpuAlloc(
                8u * sizeof(azel::DebugColorVertex),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2BarVertexUid));
    g_vdp2BarIndices =
        static_cast<std::uint16_t*>(
            probeGpuAlloc(
                12u * sizeof(std::uint16_t),
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp2BarIndexUid));

    if (!g_vdp2Nbg1AtlasPixels ||
        !g_vdp2Nbg1Vertices ||
        !g_vdp2Nbg1Indices ||
        !g_vdp2TextLayerPixels ||
        !g_vdp2TextLayerVertices ||
        !g_vdp2TextLayerIndices ||
        !g_vdp2BarVertices ||
        !g_vdp2BarIndices)
        return false;

    std::memset(
        g_vdp2Nbg1AtlasPixels, 0, atlasBytes);

    if (sceGxmTextureInitLinear(
            &g_vdp2Nbg1AtlasTexture,
            g_vdp2Nbg1AtlasPixels,
            SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
            kVdp2Nbg1AtlasWidth,
            kVdp2Nbg1AtlasHeight,
            0) < 0)
        return false;

    // Decoded 2D presentation assets are ordinary RGBA textures at this
    // point. Use SGX linear filtering when Saturn-authored UI is rescaled
    // into the active Vita framebuffer.
    sceGxmTextureSetMinFilter(
        &g_vdp2Nbg1AtlasTexture,
        SCE_GXM_TEXTURE_FILTER_LINEAR);
    sceGxmTextureSetMagFilter(
        &g_vdp2Nbg1AtlasTexture,
        SCE_GXM_TEXTURE_FILTER_LINEAR);

    std::memset(g_vdp2TextLayerPixels, 0, textLayerBytes);
    if (sceGxmTextureInitLinear(
            &g_vdp2TextLayerTexture,
            g_vdp2TextLayerPixels,
            SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
            kVdp2TextLayerWidth,
            kVdp2TextLayerHeight,
            0) < 0)
        return false;
    sceGxmTextureSetMinFilter(
        &g_vdp2TextLayerTexture,
        SCE_GXM_TEXTURE_FILTER_LINEAR);
    sceGxmTextureSetMagFilter(
        &g_vdp2TextLayerTexture,
        SCE_GXM_TEXTURE_FILTER_LINEAR);

    const std::uint16_t textIndices[6] = {0,1,2,0,2,3};
    std::memcpy(
        g_vdp2TextLayerIndices,
        textIndices,
        sizeof(textIndices));

    return true;
}

static std::uint32_t decodeVdp2Nbg1Pixel(
    std::uint16_t patternName,
    int px,
    int py)
{
    const unsigned int flip =
        (patternName >> 10) & 3u;
    int x = px;
    int y = py;

    // Match Azel's renderer_vdp2.cpp 16x16 CHSZ=1 path exactly.
    if (flip) {
        y &= 15;
        if (flip & 2u) {
            if (!(y & 8))
                y = 7 - y + 16;
            else
                y = 15 - y;
        } else if (y & 8) {
            y += 8;
        }

        if (flip & 1u) {
            if (!(x & 8))
                y += 8;
            x &= 7;
            x = 7 - x;
        } else if (x & 8) {
            y += 8;
            x &= 7;
        } else {
            x &= 7;
        }
    } else {
        y &= 15;
        if (y & 8)
            y += 8;
        if (x & 8)
            y += 8;
        x &= 7;
    }

    const unsigned int characterNumber =
        static_cast<unsigned int>(
            patternName & 0x03FFu) << 2;
    const std::size_t characterOffset =
        static_cast<std::size_t>(
            characterNumber) * 0x20u;
    const std::size_t dotOffset =
        characterOffset +
        static_cast<std::size_t>(
            (y * 8 + x) / 2);
    if (dotOffset >= kVdp2TextSnapshotBytes)
        return 0u;

    const std::uint8_t packed =
        g_vdp2TextVram[dotOffset];
    const unsigned int colorIndex =
        (x & 1)
            ? static_cast<unsigned int>(packed & 0x0Fu)
            : static_cast<unsigned int>(packed >> 4);
    if (!colorIndex)
        return 0u;

    // NBG1: 4bpp, CAOS=7, SCN=0.
    const unsigned int paladdr =
        (patternName & 0xF000u) >> 8;
    const unsigned int paletteEntry =
        7u * 0x100u |
        (paladdr | colorIndex);
    const std::size_t cramOffset =
        static_cast<std::size_t>(paletteEntry) * 2u;
    if (cramOffset + 1u >= kVdp2CramSnapshotBytes)
        return 0u;

    return vdp2Rgb555ToAbgr(
        readVdp2Be16(g_vdp2Cram, cramOffset));
}

static void drawAzelVdp2Nbg1Gpu()
{
    if (!g_vdp2TextValid ||
        !g_textureVertexProgram ||
        !g_textureFragmentProgram ||
        !g_textureWvpParam ||
        !ensureVdp2UiGpuBuffers())
        return;

    constexpr std::size_t kMapOffset = 0x5800u;
    constexpr int kMapColumns = 32;
    constexpr int kVisibleRows = 14;
    constexpr int kTileSize = 16;

    std::uint16_t uniqueTiles[kVdp2Nbg1MaxTiles]{};
    unsigned int uniqueCount = 0u;
    unsigned int cellCount = 0u;

    const float renderAspect =
        static_cast<float>(viewerRenderWidth()) /
        static_cast<float>(viewerRenderHeight());
    const float xCorrection =
        (4.0f / 3.0f) / renderAspect;

    std::memset(
        g_vdp2Nbg1AtlasPixels,
        0,
        kVdp2Nbg1AtlasWidth *
            kVdp2Nbg1AtlasHeight *
            sizeof(std::uint32_t));

    const auto findTileSlot =
        [&](std::uint16_t patternName) -> int {
            for (unsigned int i = 0; i < uniqueCount; ++i) {
                if (uniqueTiles[i] == patternName)
                    return static_cast<int>(i);
            }
            if (uniqueCount >= kVdp2Nbg1MaxTiles)
                return -1;

            const unsigned int slot = uniqueCount++;
            uniqueTiles[slot] = patternName;
            const unsigned int atlasX =
                (slot & 7u) * kTileSize;
            const unsigned int atlasY =
                (slot >> 3) * kTileSize;
            for (int py = 0; py < kTileSize; ++py) {
                for (int px = 0; px < kTileSize; ++px) {
                    g_vdp2Nbg1AtlasPixels[
                        (atlasY + py) *
                            kVdp2Nbg1AtlasWidth +
                        atlasX + px] =
                        decodeVdp2Nbg1Pixel(
                            patternName, px, py);
                }
            }
            return static_cast<int>(slot);
        };

    for (int ty = 0; ty < kVisibleRows; ++ty) {
        for (int tx = 0; tx < kMapColumns; ++tx) {
            const std::size_t mapAddress =
                kMapOffset +
                static_cast<std::size_t>(
                    (ty * kMapColumns + tx) * 2);
            const std::uint16_t patternName =
                readVdp2Be16(
                    g_vdp2TextVram, mapAddress);
            if (!patternName)
                continue;
            if (cellCount >= kVdp2Nbg1MaxCells)
                break;

            const int slot = findTileSlot(patternName);
            if (slot < 0)
                continue;

            const float sx0 =
                static_cast<float>(tx * kTileSize);
            const float sy0 =
                static_cast<float>(ty * kTileSize);
            const float sx1 = sx0 + kTileSize;
            const float sy1 = sy0 + kTileSize;

            const float x0 =
                ((sx0 - 176.0f) / 176.0f) *
                xCorrection;
            const float x1 =
                ((sx1 - 176.0f) / 176.0f) *
                xCorrection;
            const float y0 =
                1.0f - sy0 / 112.0f;
            const float y1 =
                1.0f - sy1 / 112.0f;

            const unsigned int atlasX =
                (static_cast<unsigned int>(slot) & 7u) *
                kTileSize;
            const unsigned int atlasY =
                (static_cast<unsigned int>(slot) >> 3) *
                kTileSize;
            const float u0 =
                (static_cast<float>(atlasX) + 0.5f) /
                kVdp2Nbg1AtlasWidth;
            const float v0 =
                (static_cast<float>(atlasY) + 0.5f) /
                kVdp2Nbg1AtlasHeight;
            const float u1 =
                (static_cast<float>(atlasX + kTileSize) - 0.5f) /
                kVdp2Nbg1AtlasWidth;
            const float v1 =
                (static_cast<float>(atlasY + kTileSize) - 0.5f) /
                kVdp2Nbg1AtlasHeight;

            const unsigned int base =
                cellCount * 4u;
            g_vdp2Nbg1Vertices[base + 0u] =
                {x0, y0, 0.0f, u0, v0};
            g_vdp2Nbg1Vertices[base + 1u] =
                {x1, y0, 0.0f, u1, v0};
            g_vdp2Nbg1Vertices[base + 2u] =
                {x1, y1, 0.0f, u1, v1};
            g_vdp2Nbg1Vertices[base + 3u] =
                {x0, y1, 0.0f, u0, v1};

            const unsigned int ibase =
                cellCount * 6u;
            g_vdp2Nbg1Indices[ibase + 0u] =
                static_cast<std::uint16_t>(base + 0u);
            g_vdp2Nbg1Indices[ibase + 1u] =
                static_cast<std::uint16_t>(base + 1u);
            g_vdp2Nbg1Indices[ibase + 2u] =
                static_cast<std::uint16_t>(base + 2u);
            g_vdp2Nbg1Indices[ibase + 3u] =
                static_cast<std::uint16_t>(base + 0u);
            g_vdp2Nbg1Indices[ibase + 4u] =
                static_cast<std::uint16_t>(base + 2u);
            g_vdp2Nbg1Indices[ibase + 5u] =
                static_cast<std::uint16_t>(base + 3u);
            ++cellCount;
        }
    }

    if (!cellCount)
        return;

    sceGxmSetCullMode(
        g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetFrontPolygonMode(
        g_probeContext,
        SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(
        g_probeContext,
        SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetFrontDepthFunc(
        g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(
        g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetVertexProgram(
        g_probeContext, g_textureVertexProgram);
    sceGxmSetFragmentProgram(
        g_probeContext, g_textureFragmentProgram);

    void* uniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniforms) < 0 ||
        !uniforms)
        return;

    static const float identity[16] = {
        1.0f,0.0f,0.0f,0.0f,
        0.0f,1.0f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f,0.0f,1.0f
    };
    sceGxmSetUniformDataF(
        uniforms, g_textureWvpParam,
        0, 16, identity);
    sceGxmSetVertexStream(
        g_probeContext, 0,
        g_vdp2Nbg1Vertices);
    sceGxmSetFragmentTexture(
        g_probeContext, 0,
        &g_vdp2Nbg1AtlasTexture);
    sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_vdp2Nbg1Indices,
        cellCount * 6u);
}

static void drawAzelVdp2CinematicBarsGpu()
{
    if (!g_vdp2TextValid ||
        !g_probeVertexProgram ||
        !g_probeFragmentProgram ||
        !g_probeWvpParam ||
        !ensureVdp2UiGpuBuffers())
        return;

    // Azel's cinematic-bar transfer is configured for 0x100 bytes:
    // 64 line-scroll entries. renderer_vdp2.cpp applies those entries to
    // raw output Y and then vertically flips the rendered NBG1 surface.
    // Therefore a run of 0x01010000 entries from table index 0 becomes the
    // *bottom* dialogue bar on screen. The Saturn reference shows exactly
    // this single lower bar for the Ruins interaction.
    constexpr unsigned int kTransferredLines = 0x100u / 4u;
    unsigned int barLines = 0u;
    while (barLines < kTransferredLines &&
           readVdp2Be32(
               g_vdp2LineScroll,
               barLines * 4u) == 0x01010000u)
        ++barLines;

    if (!barLines)
        return;

    // Unlike the 4:3-corrected Saturn sprites/text, the cinematic matte is
    // a presentation mask and spans the complete Vita render target.
    const float left = -1.0f;
    const float right = 1.0f;

    // Line scroll is defined in Saturn output scanlines. Preserve that
    // vertical fraction exactly; aspect correction belongs on X only.
    // PDS's cinematic line-scroll table is authored at half the displayed
    // vertical scanline cadence used by the 352-wide presentation. The live
    // table reaches 16 entries here while the Saturn reference matte covers
    // 32 of the 224 displayed lines (and the subtitle starts at y=200).
    // Expand each table entry to its two displayed scanlines.
    const unsigned int displayedBarLines =
        std::min(224u, barLines * 2u);
    const float barTop =
        -1.0f +
        static_cast<float>(displayedBarLines) / 112.0f;

    static unsigned int lastReportedBarLines = 0xFFFFFFFFu;
    if (barLines != lastReportedBarLines) {
        logging::writef(
            "[CineBar] tableLines=%u displayLines=%u top=%.4f\n",
            barLines, displayedBarLines, barTop);
        lastReportedBarLines = barLines;
    }

    g_vdp2BarVertices[0] =
        {left,  barTop, 0.0f, 0u,0u,0u,255u};
    g_vdp2BarVertices[1] =
        {right, barTop, 0.0f, 0u,0u,0u,255u};
    g_vdp2BarVertices[2] =
        {right, -1.0f,  0.0f, 0u,0u,0u,255u};
    g_vdp2BarVertices[3] =
        {left,  -1.0f,  0.0f, 0u,0u,0u,255u};

    static const std::uint16_t kBarIndices[6] = {
        0,1,2, 0,2,3
    };
    std::memcpy(
        g_vdp2BarIndices,
        kBarIndices,
        sizeof(kBarIndices));

    sceGxmSetCullMode(
        g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetFrontPolygonMode(
        g_probeContext,
        SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(
        g_probeContext,
        SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetFrontDepthFunc(
        g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(
        g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetVertexProgram(
        g_probeContext, g_probeVertexProgram);
    sceGxmSetFragmentProgram(
        g_probeContext, g_probeFragmentProgram);

    void* uniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniforms) < 0 ||
        !uniforms)
        return;
    static const float identity[16] = {
        1.0f,0.0f,0.0f,0.0f,
        0.0f,1.0f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f,0.0f,1.0f
    };
    sceGxmSetUniformDataF(
        uniforms, g_probeWvpParam,
        0, 16, identity);
    sceGxmSetVertexStream(
        g_probeContext, 0,
        g_vdp2BarVertices);
    sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_vdp2BarIndices,
        6u);
}


static GpuMode1Texture* findOrUploadVdp1UiTexture(
    const azel_bridge::Vdp1UiCommand& command)
{
    for (auto& entry : g_vdp1UiTextureCache) {
        if (entry.cmdPmod == command.cmdPmod &&
            entry.cmdColr == command.cmdColr &&
            entry.cmdSrca == command.cmdSrca &&
            entry.cmdSize == command.cmdSize)
            return &entry.gpu;
    }

    const unsigned int commandType =
        static_cast<unsigned int>(command.cmdCtrl & 0x000Fu);
    const unsigned int colorMode =
        (static_cast<unsigned int>(command.cmdPmod) >> 3) & 7u;

    // Screen-space UI commands are backed by live VDP1 VRAM rather than a
    // town CGB bundle. Decode 4bpp color-bank and color-LUT sprites directly
    // from Saturn memory for normal, scaled and distorted sprite commands.
    // Geometry type does not change the texture encoding.
    if ((commandType == 0u || commandType == 1u || commandType == 2u) &&
        (colorMode == 0u || colorMode == 1u)) {
        const unsigned int width =
            ((static_cast<unsigned int>(command.cmdSize) >> 8) & 0x3Fu) * 8u;
        const unsigned int height =
            static_cast<unsigned int>(command.cmdSize) & 0xFFu;
        if (!width || !height)
            return nullptr;

        const unsigned int texBytes = (width * height) / 2u;
        const unsigned int texAddress =
            static_cast<unsigned int>(command.cmdSrca) << 3;
        if (texAddress + texBytes > 0x80000u)
            return nullptr;

        const unsigned char* const src =
            getVdp1Pointer(0x25C00000u + texAddress);
        if (!src)
            return nullptr;

        azel::DecodedMode1Texture decoded{};
        decoded.cmdPmod = command.cmdPmod;
        decoded.cmdColr = command.cmdColr;
        decoded.cmdSrca = command.cmdSrca;
        decoded.cmdSize = command.cmdSize;
        decoded.width = width;
        decoded.height = height;
        decoded.rgba.assign(width * height, 0u);

        const bool spd = (command.cmdPmod & 0x40u) != 0u;
        const bool endDisabled = (command.cmdPmod & 0x80u) != 0u;
        const bool endMode = (command.cmdPmod & 0x20u) == 0u;
        unsigned int pixel = 0u;
        for (unsigned int y = 0; y < height; ++y) {
            unsigned int endCount = 0u;
            for (unsigned int x = 0; x < width; ++x, ++pixel) {
                const unsigned char packed =
                    src[(x + y * width) / 2u];
                const unsigned int dot =
                    (x & 1u) ? (packed & 0x0Fu) : (packed >> 4);

                if (endMode && endCount >= 2u)
                    continue;
                if (dot == 0u && !spd)
                    continue;
                if (dot == 0x0Fu && !endDisabled) {
                    ++endCount;
                    continue;
                }

                std::uint16_t color = 0u;
                if (colorMode == 0u) {
                    const unsigned int paletteIndex =
                        (static_cast<unsigned int>(command.cmdColr) & 0x07F0u) |
                        dot;
                    const unsigned int cramByte = paletteIndex * 2u;
                    if (cramByte + 1u >= sizeof(g_vdp2Cram))
                        continue;
                    color = readVdp2Be16(g_vdp2Cram, cramByte);
                } else {
                    // VDP1 color-LUT mode: CMDCOLR is the LUT address in
                    // 8-byte units, and each 4bpp source dot selects one BE16
                    // LUT entry. Direct RGB555 entries carry bit 15; indirect
                    // entries address CRAM.
                    const unsigned int lutAddress =
                        (static_cast<unsigned int>(command.cmdColr) << 3) +
                        dot * 2u;
                    if (lutAddress + 1u >= 0x80000u)
                        continue;
                    const unsigned char* const lut =
                        getVdp1Pointer(0x25C00000u + lutAddress);
                    if (!lut)
                        continue;
                    const std::uint16_t lutValue =
                        static_cast<std::uint16_t>(
                            (static_cast<unsigned int>(lut[0]) << 8) |
                            static_cast<unsigned int>(lut[1]));
                    if (lutValue & 0x8000u) {
                        color = lutValue;
                    } else {
                        const unsigned int cramByte =
                            (static_cast<unsigned int>(lutValue) & 0x07FFu) * 2u;
                        if (cramByte + 1u >= sizeof(g_vdp2Cram))
                            continue;
                        color = readVdp2Be16(g_vdp2Cram, cramByte);
                    }
                }
                if (color)
                    decoded.rgba[pixel] = vdp2Rgb555ToAbgr(color);
            }
        }

        const unsigned int stridePixels =
            (decoded.width + 7u) & ~7u;
        const unsigned int bytes =
            stridePixels * decoded.height * sizeof(std::uint32_t);

        Vdp1UiTextureCacheEntry entry{};
        entry.cmdPmod = command.cmdPmod;
        entry.cmdColr = command.cmdColr;
        entry.cmdSrca = command.cmdSrca;
        entry.cmdSize = command.cmdSize;
        entry.gpu.data = probeGpuAlloc(
            bytes, SCE_GXM_MEMORY_ATTRIB_READ, &entry.gpu.uid);
        if (!entry.gpu.data)
            return nullptr;
        entry.gpu.width = decoded.width;
        entry.gpu.height = decoded.height;
        std::memset(entry.gpu.data, 0, bytes);
        auto* dst = static_cast<std::uint32_t*>(entry.gpu.data);
        for (unsigned int y = 0; y < decoded.height; ++y) {
            std::memcpy(
                dst + y * stridePixels,
                decoded.rgba.data() + y * decoded.width,
                decoded.width * sizeof(std::uint32_t));
        }

        if (sceGxmTextureInitLinear(
                &entry.gpu.texture,
                entry.gpu.data,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
                decoded.width,
                decoded.height,
                0) < 0) {
            sceGxmUnmapMemory(entry.gpu.data);
            sceKernelFreeMemBlock(entry.gpu.uid);
            return nullptr;
        }
        sceGxmTextureSetMinFilter(
            &entry.gpu.texture, SCE_GXM_TEXTURE_FILTER_LINEAR);
        sceGxmTextureSetMagFilter(
            &entry.gpu.texture, SCE_GXM_TEXTURE_FILTER_LINEAR);

        static bool reportedLiveUiSprite = false;
        if (!reportedLiveUiSprite) {
            unsigned int sourceNonZero = 0u;
            unsigned int visiblePixels = 0u;
            std::uint32_t sourceHash = 2166136261u;
            std::uint32_t rgbaHash = 2166136261u;
            for (unsigned int i = 0; i < texBytes; ++i) {
                if (src[i] != 0u)
                    ++sourceNonZero;
                sourceHash ^= src[i];
                sourceHash *= 16777619u;
            }
            for (const auto px : decoded.rgba) {
                if ((px >> 24) != 0u)
                    ++visiblePixels;
                rgbaHash ^= px;
                rgbaHash *= 16777619u;
            }
            const unsigned int base =
                static_cast<unsigned int>(command.cmdColr) & 0x07F0u;
            logging::writef(
                "[VDP1LiveUI] type=%u mode=%u SRCA=%04X SIZE=%04X COLR=%04X %ux%u "
                "srcNZ=%u/%u vis=%u/%u srcHash=%08X rgbaHash=%08X "
                "pal=%04X:%04X,%04X,%04X,%04X\n",
                commandType,
                colorMode,
                static_cast<unsigned int>(command.cmdSrca),
                static_cast<unsigned int>(command.cmdSize),
                static_cast<unsigned int>(command.cmdColr),
                width, height,
                sourceNonZero, texBytes,
                visiblePixels,
                static_cast<unsigned int>(decoded.rgba.size()),
                sourceHash, rgbaHash,
                static_cast<unsigned int>(
                    readVdp2Be16(g_vdp2Cram, ((base | 1u) * 2u) & 0x0FFFu)),
                static_cast<unsigned int>(
                    readVdp2Be16(g_vdp2Cram, ((base | 2u) * 2u) & 0x0FFFu)),
                static_cast<unsigned int>(
                    readVdp2Be16(g_vdp2Cram, ((base | 3u) * 2u) & 0x0FFFu)),
                static_cast<unsigned int>(
                    readVdp2Be16(g_vdp2Cram, ((base | 4u) * 2u) & 0x0FFFu)));
            reportedLiveUiSprite = true;
        }

        g_vdp1UiTextureCache.push_back(std::move(entry));
        return &g_vdp1UiTextureCache.back().gpu;
    }

    azel::SaturnPolygonRecord record{};
    record.cmdCtrl = command.cmdCtrl;
    record.cmdPmod = command.cmdPmod;
    record.cmdColr = command.cmdColr;
    record.cmdSrca = command.cmdSrca;
    record.cmdSize = command.cmdSize;

    azel::DecodedMode1Texture decoded{};
    if (!azel::decode_town_texture_descriptor(record, decoded) ||
        !decoded.width || !decoded.height ||
        decoded.rgba.size() != decoded.width * decoded.height)
        return nullptr;

    const unsigned int stridePixels = (decoded.width + 7u) & ~7u;
    const unsigned int bytes =
        stridePixels * decoded.height * sizeof(std::uint32_t);

    Vdp1UiTextureCacheEntry entry{};
    entry.cmdPmod = command.cmdPmod;
    entry.cmdColr = command.cmdColr;
    entry.cmdSrca = command.cmdSrca;
    entry.cmdSize = command.cmdSize;
    entry.gpu.data = probeGpuAlloc(
        bytes, SCE_GXM_MEMORY_ATTRIB_READ, &entry.gpu.uid);
    if (!entry.gpu.data)
        return nullptr;

    entry.gpu.width = decoded.width;
    entry.gpu.height = decoded.height;
    std::memset(entry.gpu.data, 0, bytes);
    auto* dst = static_cast<std::uint32_t*>(entry.gpu.data);
    for (unsigned int y = 0; y < decoded.height; ++y) {
        std::memcpy(
            dst + y * stridePixels,
            decoded.rgba.data() + y * decoded.width,
            decoded.width * sizeof(std::uint32_t));
    }

    if (sceGxmTextureInitLinear(
            &entry.gpu.texture,
            entry.gpu.data,
            SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
            decoded.width,
            decoded.height,
            0) < 0) {
        sceGxmUnmapMemory(entry.gpu.data);
        sceKernelFreeMemBlock(entry.gpu.uid);
        return nullptr;
    }
    sceGxmTextureSetMinFilter(
        &entry.gpu.texture, SCE_GXM_TEXTURE_FILTER_LINEAR);
    sceGxmTextureSetMagFilter(
        &entry.gpu.texture, SCE_GXM_TEXTURE_FILTER_LINEAR);

    g_vdp1UiTextureCache.push_back(std::move(entry));
    return &g_vdp1UiTextureCache.back().gpu;
}

static void drawPublishedVdp1Ui()
{
    const auto& commands = azel_bridge::published_vdp1_ui_commands();
    if (commands.empty() || !g_textureVertexProgram ||
        !g_textureFragmentProgram || !g_textureWvpParam ||
        !ensureVdp1UiBuffers())
        return;

    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    // Screen-space VDP1 UI must not inherit polygon rasterization state from
    // the preceding 3D diagnostic mode.
    sceGxmSetFrontPolygonMode(
        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(
        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetFrontDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);

    sceGxmSetVertexProgram(g_probeContext, g_textureVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext, g_textureFragmentProgram);

    void* uniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniforms) < 0 || !uniforms)
        return;
    static const float identity[16] = {
        1.0f,0.0f,0.0f,0.0f,
        0.0f,1.0f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f,0.0f,1.0f
    };
    sceGxmSetUniformDataF(
        uniforms, g_textureWvpParam, 0, 16, identity);

    static unsigned int uiTraceBudget = 48u;
    if (uiTraceBudget != 0u) {
        logging::writef(
            "[PresentationTrace][NeptuneUI] commands=%u render=%dx%d\n",
            static_cast<unsigned int>(commands.size()),
            viewerRenderWidth(),
            viewerRenderHeight());
    }

    unsigned int spriteSlot = 0u;
    for (const auto& command : commands) {
        const unsigned int commandType = command.cmdCtrl & 0x000Fu;

        const float renderAspect =
            static_cast<float>(viewerRenderWidth()) /
            static_cast<float>(viewerRenderHeight());
        const float saturnAspectCorrection =
            (4.0f / 3.0f) / renderAspect;

        // VDP1 polyline. Town LCS uses this for the shrinking white
        // selection rectangle before the steady-state cursor/target sprites.
        if (commandType == 0x0005u) {
            if (!g_probeVertexProgram || !g_probeFragmentProgram ||
                !g_probeWvpParam)
                continue;

            const std::uint16_t c = command.cmdColr;
            const std::uint8_t r = static_cast<std::uint8_t>(
                (((c >> 0) & 31u) << 3) | (((c >> 0) & 31u) >> 2));
            const std::uint8_t g = static_cast<std::uint8_t>(
                (((c >> 5) & 31u) << 3) | (((c >> 5) & 31u) >> 2));
            const std::uint8_t b = static_cast<std::uint8_t>(
                (((c >> 10) & 31u) << 3) | (((c >> 10) & 31u) >> 2));

            const float x[4] = {
                (static_cast<float>(command.xa) / 176.0f) *
                    saturnAspectCorrection,
                (static_cast<float>(command.xb) / 176.0f) *
                    saturnAspectCorrection,
                (static_cast<float>(command.xc) / 176.0f) *
                    saturnAspectCorrection,
                (static_cast<float>(command.xd) / 176.0f) *
                    saturnAspectCorrection
            };
            const float y[4] = {
                -static_cast<float>(command.ya) / 112.0f,
                -static_cast<float>(command.yb) / 112.0f,
                -static_cast<float>(command.yc) / 112.0f,
                -static_cast<float>(command.yd) / 112.0f
            };
            for (unsigned int i = 0; i < 4u; ++i)
                g_vdp1UiLineVertices[i] = {
                    x[i], y[i], 0.0f, r, g, b, 255u
                };

            sceGxmSetVertexProgram(g_probeContext, g_probeVertexProgram);
            sceGxmSetFragmentProgram(
                g_probeContext, g_probeFragmentProgram);
            sceGxmSetFrontPolygonMode(
                g_probeContext, SCE_GXM_POLYGON_MODE_LINE);
            sceGxmSetBackPolygonMode(
                g_probeContext, SCE_GXM_POLYGON_MODE_LINE);
            void* lineUniforms = nullptr;
            if (sceGxmReserveVertexDefaultUniformBuffer(
                    g_probeContext, &lineUniforms) >= 0 && lineUniforms) {
                sceGxmSetUniformDataF(
                    lineUniforms, g_probeWvpParam, 0, 16, identity);
                sceGxmSetVertexStream(
                    g_probeContext, 0, g_vdp1UiLineVertices);
                sceGxmDraw(
                    g_probeContext,
                    SCE_GXM_PRIMITIVE_LINES,
                    SCE_GXM_INDEX_FORMAT_U16,
                    g_vdp1UiLineIndices,
                    8);
            }

            // Restore the textured UI pipeline for subsequent sprite commands.
            sceGxmSetFrontPolygonMode(
                g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
            sceGxmSetBackPolygonMode(
                g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
            sceGxmSetVertexProgram(
                g_probeContext, g_textureVertexProgram);
            sceGxmSetFragmentProgram(
                g_probeContext, g_textureFragmentProgram);
            void* textureUniforms = nullptr;
            if (sceGxmReserveVertexDefaultUniformBuffer(
                    g_probeContext, &textureUniforms) >= 0 &&
                textureUniforms) {
                sceGxmSetUniformDataF(
                    textureUniforms,
                    g_textureWvpParam,
                    0, 16, identity);
            }
            continue;
        }

        // Town Lock-On uses scaled sprites (type 1), while Azel's
        // multi-choice cursor uses a normal VDP1 sprite (type 0). Both are
        // authentic Azel commands and share the same decoded texture path.
        if ((commandType != 0x0000u &&
             commandType != 0x0001u &&
             commandType != 0x0002u) ||
            ((command.cmdCtrl >> 8) & 0xFu) != 0u ||
            command.cmdSrca == 0u)
            continue;

        GpuMode1Texture* texture =
            findOrUploadVdp1UiTexture(command);
        if (!texture) {
            if (uiTraceBudget != 0u) {
                logging::writef(
                    "[PresentationTrace][NeptuneUI] DROP type=%u CTRL=%04X "
                    "PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X reason=texture\n",
                    commandType,
                    command.cmdCtrl,
                    command.cmdPmod,
                    command.cmdColr,
                    command.cmdSrca,
                    command.cmdSize);
                --uiTraceBudget;
            }
            continue;
        }

        if (uiTraceBudget != 0u) {
            logging::writef(
                "[PresentationTrace][NeptuneUI] DRAW type=%u CTRL=%04X "
                "PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X "
                "A=(%d,%d) B=(%d,%d)\n",
                commandType,
                command.cmdCtrl,
                command.cmdPmod,
                command.cmdColr,
                command.cmdSrca,
                command.cmdSize,
                command.xa, command.ya,
                command.xb, command.yb);
            --uiTraceBudget;
        }

        // FLD_A3's 48x48 radar sphere deliberately uses alternating
        // transparent texels (Saturn mesh-style translucency). Preserve that
        // pattern at one source texel per output pixel with point sampling.
        const bool fieldRadarMap =
            g_sceneGameMode == 3u &&
            commandType == 0x0000u &&
            command.cmdPmod == 0x0088u &&
            command.cmdSize == 0x0630u;

        // Match the horizontal presentation transform used by
        // buildAzelProjection(). Azel emits centered 352x224 VDP1
        // coordinates, while Neptune presents that Saturn-authored image in
        // a centered 4:3 region of the Vita framebuffer.
        const float x0 =
            (static_cast<float>(command.xa) / 176.0f) *
            saturnAspectCorrection;
        const float y0 =
            -static_cast<float>(command.ya) / 112.0f;

        float x1 = x0;
        float y1 = y0;
        if (commandType == 0x0001u) {
            // Scaled sprite: Azel supplies the opposite corner directly.
            x1 =
                (static_cast<float>(command.xc + 1) / 176.0f) *
                saturnAspectCorrection;
            y1 =
                -static_cast<float>(command.yc + 1) / 112.0f;
        } else {
            // Normal sprite: CMDSIZE stores width in 8-pixel units in the
            // upper byte and height in pixels in the lower byte. XA/YA is
            // the sprite origin; XB..YD are not geometry for this command.
            const unsigned int spriteWidth =
                ((static_cast<unsigned int>(command.cmdSize) >> 8) &
                 0x3Fu) * 8u;
            const unsigned int spriteHeight =
                static_cast<unsigned int>(command.cmdSize) & 0xFFu;
            if (!spriteWidth || !spriteHeight)
                continue;

            if (fieldRadarMap) {
                x1 = x0 +
                    (2.0f * static_cast<float>(spriteWidth)) /
                    static_cast<float>(viewerRenderWidth());
                y1 = y0 -
                    (2.0f * static_cast<float>(spriteHeight)) /
                    static_cast<float>(viewerRenderHeight());
            } else {
                x1 =
                    (static_cast<float>(
                        command.xa +
                        static_cast<std::int16_t>(spriteWidth)) /
                     176.0f) *
                    saturnAspectCorrection;
                y1 =
                    -static_cast<float>(
                        command.ya +
                        static_cast<std::int16_t>(spriteHeight)) /
                    112.0f;
            }
        }

        const float u0 = 0.5f / static_cast<float>(texture->width);
        const float v0 = 0.5f / static_cast<float>(texture->height);
        const float u1 =
            (static_cast<float>(texture->width) - 0.5f) /
            static_cast<float>(texture->width);
        const float v1 =
            (static_cast<float>(texture->height) - 0.5f) /
            static_cast<float>(texture->height);
        const float uv[4][2] = {
            {u0,v0}, {u1,v0}, {u1,v1}, {u0,v1}
        };

        if (commandType == 0x0000u) {
            static bool reportedNormalGeometry = false;
            if (!reportedNormalGeometry) {
                logging::writef(
                    "[VDP1NormalGeom] A=(%d,%d) ndc=(%.4f,%.4f)-(%.4f,%.4f) CTRL=%04X\n",
                    static_cast<int>(command.xa),
                    static_cast<int>(command.ya),
                    x0, y0, x1, y1,
                    static_cast<unsigned int>(command.cmdCtrl));
                reportedNormalGeometry = true;
            }
        }
        int order[4] = {0,1,2,3};
        switch ((command.cmdCtrl >> 4) & 3u) {
        case 1:
            order[0]=1; order[1]=0; order[2]=3; order[3]=2;
            break;
        case 2:
            order[0]=3; order[1]=2; order[2]=1; order[3]=0;
            break;
        case 3:
            order[0]=2; order[1]=3; order[2]=0; order[3]=1;
            break;
        default:
            break;
        }

        if (spriteSlot >= 128u)
            continue;

        azel::DebugTextureVertex* const spriteVertices =
            g_vdp1UiVertices + spriteSlot * 4u;
        ++spriteSlot;

        float pos[4][2] = {
            {x0,y0}, {x1,y0}, {x1,y1}, {x0,y1}
        };
        if (commandType == 0x0002u) {
            // Distorted sprite: Azel supplies all four projected corners.
            // D5's native particles and name-entry exit sprite use this path.
            pos[0][0] = (static_cast<float>(command.xa) / 176.0f) *
                saturnAspectCorrection;
            pos[0][1] = -static_cast<float>(command.ya) / 112.0f;
            pos[1][0] = (static_cast<float>(command.xb) / 176.0f) *
                saturnAspectCorrection;
            pos[1][1] = -static_cast<float>(command.yb) / 112.0f;
            pos[2][0] = (static_cast<float>(command.xc) / 176.0f) *
                saturnAspectCorrection;
            pos[2][1] = -static_cast<float>(command.yc) / 112.0f;
            pos[3][0] = (static_cast<float>(command.xd) / 176.0f) *
                saturnAspectCorrection;
            pos[3][1] = -static_cast<float>(command.yd) / 112.0f;
        }
        for (unsigned int i = 0; i < 4u; ++i) {
            spriteVertices[i] = {
                pos[i][0], pos[i][1], 0.0f,
                uv[order[i]][0], uv[order[i]][1]
            };
        }

        // GXM consumes vertex streams asynchronously. Never overwrite a
        // vertex slice after issuing its draw within the same scene; the next
        // VDP1 sprite gets a separate 4-vertex region.
        sceGxmSetVertexStream(g_probeContext, 0, spriteVertices);
        if (fieldRadarMap) {
            sceGxmTextureSetMinFilter(
                &texture->texture, SCE_GXM_TEXTURE_FILTER_POINT);
            sceGxmTextureSetMagFilter(
                &texture->texture, SCE_GXM_TEXTURE_FILTER_POINT);
        }
        sceGxmSetFragmentTexture(
            g_probeContext, 0, &texture->texture);
        sceGxmDraw(
            g_probeContext,
            SCE_GXM_PRIMITIVE_TRIANGLES,
            SCE_GXM_INDEX_FORMAT_U16,
            g_vdp1UiIndices,
            6);
        if (fieldRadarMap) {
            sceGxmTextureSetMinFilter(
                &texture->texture, SCE_GXM_TEXTURE_FILTER_LINEAR);
            sceGxmTextureSetMagFilter(
                &texture->texture, SCE_GXM_TEXTURE_FILTER_LINEAR);
        }
    }

}

static bool uploadVdp1Textures(const Vdp1ModelSource& model)
{
    freeVdp1Textures();

    if (!model.valid())
        return false;

    g_vdp1GpuTextures.reserve(model.textureCount);

    for (std::size_t textureIndex = 0;
         textureIndex < model.textureCount; ++textureIndex) {
        const auto& source = model.textures[textureIndex];
        if (!source.width || !source.height ||
            source.rgba.size() != source.width * source.height)
            return false;

        const unsigned int stridePixels = (source.width + 7u) & ~7u;
        const unsigned int bytes =
            stridePixels * source.height * sizeof(std::uint32_t);

        GpuMode1Texture gpu{};
        gpu.data = probeGpuAlloc(
            bytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &gpu.uid);
        if (!gpu.data)
            return false;

        gpu.width = source.width;
        gpu.height = source.height;
        gpu.opaque = std::all_of(
            source.rgba.begin(), source.rgba.end(),
            [](std::uint32_t pixel) {
                return (pixel >> 24) >= 0x80u;
            });
        gpu.mesh = (source.cmdPmod & 0x0100u) != 0u;
        std::memset(gpu.data, 0, bytes);

        auto* dst = static_cast<std::uint32_t*>(gpu.data);
        for (unsigned int y = 0; y < source.height; ++y) {
            std::memcpy(
                dst + y * stridePixels,
                source.rgba.data() + y * source.width,
                source.width * sizeof(std::uint32_t));
        }

        if (sceGxmTextureInitLinear(
                &gpu.texture,
                gpu.data,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,
                source.width,
                source.height,
                0) < 0) {
            sceGxmUnmapMemory(gpu.data);
            sceKernelFreeMemBlock(gpu.uid);
            return false;
        }

        sceGxmTextureSetMinFilter(
            &gpu.texture, SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(
            &gpu.texture, SCE_GXM_TEXTURE_FILTER_POINT);

        g_vdp1GpuTextures.push_back(gpu);
    }

    return g_vdp1GpuTextures.size() == model.textureCount;
}

static bool buildVdp1TexturedBuffers(
    const Vdp1ModelSource& model,
    bool reuseMappedBuffers)
{
    if (!model.valid() || model.vertexCount > 65535u ||
        g_vdp1GpuTextures.empty())
        return false;

    const unsigned int vertexCount =
        static_cast<unsigned int>(model.vertexCount);

    const unsigned int textureVertexBytes =
        vertexCount * sizeof(azel::DebugTextureVertex);
    const unsigned int gouraudVertexBytes =
        vertexCount * sizeof(azel::DebugGouraudPayloadVertex);
    const unsigned int indexBytes =
        vertexCount * sizeof(std::uint16_t);

    if (!reuseMappedBuffers) {
        const std::uint64_t allocStartUs = sceKernelGetProcessTimeWide();
        g_vdp1TextureVertices =
            static_cast<azel::DebugTextureVertex*>(
                probeGpuAlloc(
                    textureVertexBytes,
                    SCE_GXM_MEMORY_ATTRIB_READ,
                    &g_vdp1TextureVertexUid));
        g_vdp1GouraudVertices =
            static_cast<azel::DebugGouraudPayloadVertex*>(
                probeGpuAlloc(
                    gouraudVertexBytes,
                    SCE_GXM_MEMORY_ATTRIB_READ,
                    &g_vdp1GouraudVertexUid));
        g_vdp1TextureIndices =
            static_cast<std::uint16_t*>(
                probeGpuAlloc(
                    indexBytes,
                    SCE_GXM_MEMORY_ATTRIB_READ,
                    &g_vdp1TextureIndexUid));
        g_profilePrepareTexturedAllocUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - allocStartUs);
        g_vdp1TextureVertexCapacity = vertexCount;
        g_vdp1TextureIndexCapacity = vertexCount;
    }

    if (!g_vdp1TextureVertices ||
        !g_vdp1GouraudVertices ||
        !g_vdp1TextureIndices)
        return false;

    static const int triCorners[6] = {0, 1, 2, 0, 2, 3};
    const std::uint64_t buildStartUs = sceKernelGetProcessTimeWide();

    for (unsigned int p = 0; p < model.polygonCount; ++p) {
        const auto& record = model.polygons[p];
        const std::uint16_t textureIndex =
            model.polygonTextureIndices[p];
        if (textureIndex >= model.textureCount)
            return false;

        const auto& texture =
            model.textures[textureIndex];

        const float u0 = 0.5f / static_cast<float>(texture.width);
        const float v0 = 0.5f / static_cast<float>(texture.height);
        const float u1 =
            (static_cast<float>(texture.width) - 0.5f) /
            static_cast<float>(texture.width);
        const float v1 =
            (static_cast<float>(texture.height) - 0.5f) /
            static_cast<float>(texture.height);

        const float uv[4][2] = {
            {u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}
        };

        int order[4] = {0, 1, 2, 3};
        switch (record.textureFlip() & 3u) {
            case 1:
                order[0] = 1; order[1] = 0;
                order[2] = 3; order[3] = 2;
                break;
            case 2:
                order[0] = 3; order[1] = 2;
                order[2] = 1; order[3] = 0;
                break;
            case 3:
                order[0] = 2; order[1] = 3;
                order[2] = 0; order[3] = 1;
                break;
            default:
                break;
        }

        for (unsigned int k = 0; k < 6; ++k) {
            const unsigned int vertexIndex = p * 6u + k;
            const auto& source =
                model.vertices[vertexIndex];
            const int corner = order[triCorners[k]];

            g_vdp1TextureVertices[vertexIndex] = {
                source.x, source.y, source.z,
                uv[corner][0], uv[corner][1]
            };

            auto& gouraudVertex =
                g_vdp1GouraudVertices[vertexIndex];
            gouraudVertex = {};
            gouraudVertex.x = source.x;
            gouraudVertex.y = source.y;
            gouraudVertex.z = source.z;
            gouraudVertex.u = uv[corner][0];
            gouraudVertex.v = uv[corner][1];
        }
    }

    // Build identical per-texture batches in O(polygons + textures). The old
    // implementation rescanned every polygon once for every resident texture,
    // which made a field visibility change roughly 900 x 1,000 comparisons
    // before writing the same indices. Counting and prefixing retain original
    // polygon order within each texture batch without the quadratic scan.
    g_vdp1TextureBatches.assign(
        g_vdp1GpuTextures.size(), TextureBatch{});
    for (unsigned int p = 0; p < model.polygonCount; ++p) {
        const std::uint16_t textureIndex = model.polygonTextureIndices[p];
        if (textureIndex >= g_vdp1TextureBatches.size())
            return false;
        g_vdp1TextureBatches[textureIndex].indexCount += 6u;
    }

    unsigned int outIndex = 0u;
    std::vector<unsigned int> writeOffsets(g_vdp1TextureBatches.size());
    for (unsigned int t = 0; t < g_vdp1TextureBatches.size(); ++t) {
        TextureBatch& batch = g_vdp1TextureBatches[t];
        batch.firstIndex = outIndex;
        writeOffsets[t] = outIndex;
        outIndex += batch.indexCount;
    }

    if (outIndex != vertexCount)
        return false;

    for (unsigned int p = 0; p < model.polygonCount; ++p) {
        const std::uint16_t textureIndex = model.polygonTextureIndices[p];
        unsigned int& write = writeOffsets[textureIndex];
        for (unsigned int k = 0; k < 6; ++k)
            g_vdp1TextureIndices[write++] =
                static_cast<std::uint16_t>(p * 6u + k);
    }


    g_profilePrepareTexturedBuildUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - buildStartUs);
    return true;
}

static void releaseResidentVdp1Model(bool preserveTextures = false)
{
    auto freeMapped = [](SceUID& uid, void*& ptr) {
        if (uid >= 0) {
            void* mem = nullptr;
            if (sceKernelGetMemBlockBase(uid, &mem) >= 0 && mem)
                sceGxmUnmapMemory(mem);
            sceKernelFreeMemBlock(uid);
        }
        uid = -1;
        ptr = nullptr;
    };

    void* p = g_vdp1Vertices;
    freeMapped(g_vdp1VertexUid, p);
    g_vdp1Vertices = nullptr;

    p = g_vdp1LightingVertices;
    freeMapped(g_vdp1LightingVertexUid, p);
    g_vdp1LightingVertices = nullptr;

    p = g_vdp1Indices;
    freeMapped(g_vdp1IndexUid, p);
    g_vdp1Indices = nullptr;
    g_vdp1VertexCapacity = 0u;
    g_vdp1IndexCapacity = 0u;

    p = g_vdp1TextureVertices;
    freeMapped(g_vdp1TextureVertexUid, p);
    g_vdp1TextureVertices = nullptr;

    p = g_vdp1GouraudVertices;
    freeMapped(g_vdp1GouraudVertexUid, p);
    g_vdp1GouraudVertices = nullptr;

    p = g_vdp1TextureIndices;
    freeMapped(g_vdp1TextureIndexUid, p);
    g_vdp1TextureIndices = nullptr;
    g_vdp1TextureVertexCapacity = 0u;
    g_vdp1TextureIndexCapacity = 0u;

    p = g_vdp1SubdivVertices;
    freeMapped(g_vdp1SubdivVertexUid, p);
    g_vdp1SubdivVertices = nullptr;

    p = g_vdp1SubdivIndices;
    freeMapped(g_vdp1SubdivIndexUid, p);
    g_vdp1SubdivIndices = nullptr;
    g_vdp1SubdivVertexCapacity = 0u;
    g_vdp1SubdivIndexCapacity = 0u;
    g_vdp1SubdivQuadIndices.clear();

    p = g_vdp1SubdivWireVertices;
    freeMapped(g_vdp1SubdivWireVertexUid, p);
    g_vdp1SubdivWireVertices = nullptr;

    p = g_vdp1SubdivWireIndices;
    freeMapped(g_vdp1SubdivWireIndexUid, p);
    g_vdp1SubdivWireIndices = nullptr;
    g_vdp1SubdivWireVertexCapacity = 0u;
    g_vdp1SubdivWireIndexCapacity = 0u;

    if (!preserveTextures)
        freeVdp1Textures();
    g_vdp1TextureBatches.clear();
    g_vdp1TexturedReady = false;
    g_residentVdp1Model = ResidentVdp1Model::None;
}

bool prepare_vdp1_model(
    const Vdp1ModelSource& model,
    bool preserveResidentTextures)
{
    if (!g_gxmInitialized || !g_probeContext || !model.valid())
        return false;

    // Field visibility changes alter the flattened geometry frequently.
    // Reuse the already-uploaded texture set when it is still byte-for-byte
    // valid; only geometry buffers/topology need to be rebuilt in that case.
    const bool reuseTextures =
        preserveResidentTextures &&
        !g_vdp1TextureDataDirty &&
        model.texturesValid() &&
        !g_vdp1GpuTextures.empty() &&
        g_vdp1GpuTextures.size() == model.textureCount;

    if (model.vertexCount > 65535u)
        return false;

    const unsigned int vertexCount =
        static_cast<unsigned int>(model.vertexCount);
    const unsigned int vertexBytes =
        vertexCount * sizeof(azel::DebugColorVertex);
    // Filled geometry needs six indices per source quad. The Wires diagnostic
    // follows the original Saturn quad perimeter directly, which needs four
    // independent line segments (eight indices) per quad.
    const std::size_t wireIndexCount = model.polygonCount * 8u;
    const std::size_t genericIndexCount =
        std::max<std::size_t>(model.vertexCount, wireIndexCount);
    const unsigned int indexBytes =
        static_cast<unsigned int>(
            genericIndexCount * sizeof(std::uint16_t));
    const std::size_t subdivWireVertexCount = model.polygonCount * 4u;
    const std::size_t subdivVertexCount = model.polygonCount * 9u;
    const std::size_t subdivIndexCount = model.polygonCount * 24u;
    const bool wireBuffersRequired =
        subdivWireVertexCount && subdivWireVertexCount <= 65535u;
    const bool subdivBuffersRequired =
        model.texturesValid() && subdivVertexCount &&
        subdivVertexCount <= 65535u && subdivIndexCount <= 65535u;

    // The live field mesh remains a temporary flattened resource, but a
    // visibility change should not churn mapped GXM buffers when the new set
    // fits the capacity already resident. This is independent of texture
    // residency: a CRAM/VDP1 texture invalidation can refresh textures while
    // retaining compatible geometry allocations.
    const bool reuseGeometry =
        preserveResidentTextures &&
        g_vdp1Vertices && g_vdp1LightingVertices && g_vdp1Indices &&
        g_vdp1VertexCapacity >= vertexCount &&
        g_vdp1IndexCapacity >= genericIndexCount &&
        (!model.texturesValid() ||
            (g_vdp1TextureVertices && g_vdp1GouraudVertices &&
             g_vdp1TextureIndices &&
             g_vdp1TextureVertexCapacity >= vertexCount &&
             g_vdp1TextureIndexCapacity >= vertexCount)) &&
        (!wireBuffersRequired ||
            (g_vdp1SubdivWireVertices && g_vdp1SubdivWireIndices &&
             g_vdp1SubdivWireVertexCapacity >= subdivWireVertexCount &&
             g_vdp1SubdivWireIndexCapacity >= subdivWireVertexCount)) &&
        (!subdivBuffersRequired ||
            (g_vdp1SubdivVertices && g_vdp1SubdivIndices &&
             g_vdp1SubdivVertexCapacity >= subdivVertexCount &&
             g_vdp1SubdivIndexCapacity >= subdivIndexCount));

    g_profilePrepareReleaseUs = 0u;
    g_profilePrepareBaseAllocUs = 0u;
    g_profilePrepareWireUs = 0u;
    g_profilePrepareTextureUploadUs = 0u;
    g_profilePrepareTexturedAllocUs = 0u;
    g_profilePrepareTexturedBuildUs = 0u;
    g_profilePrepareSubdivAllocUs = 0u;
    g_profilePrepareSubdivBuildUs = 0u;
    g_profilePrepareCopyUs = 0u;
    g_profilePrepareReusedTextures = reuseTextures;
    g_profilePrepareReusedGeometry = reuseGeometry;

    // The viewer still keeps one resident geometry set at a time. Geometry
    // and textures can be retained independently for the field streaming path.
    const std::uint64_t releaseStartUs = sceKernelGetProcessTimeWide();
    if (!reuseGeometry &&
        (g_vdp1Vertices || g_vdp1LightingVertices || g_vdp1Indices ||
        g_vdp1TextureVertices || g_vdp1GouraudVertices ||
        g_vdp1TextureIndices || g_vdp1SubdivVertices ||
        g_vdp1SubdivIndices || g_vdp1SubdivWireVertices ||
        g_vdp1SubdivWireIndices || (!reuseTextures && !g_vdp1GpuTextures.empty())))
        releaseResidentVdp1Model(reuseTextures);
    g_profilePrepareReleaseUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - releaseStartUs);

    if (!reuseGeometry) {
        const std::uint64_t baseAllocStartUs = sceKernelGetProcessTimeWide();
        g_vdp1Vertices = static_cast<azel::DebugColorVertex*>(
            probeGpuAlloc(
                vertexBytes,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp1VertexUid));
        g_vdp1LightingVertices = static_cast<azel::DebugColorVertex*>(
            probeGpuAlloc(
                vertexBytes,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp1LightingVertexUid));
        g_vdp1Indices = static_cast<std::uint16_t*>(
            probeGpuAlloc(
                indexBytes,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp1IndexUid));
        g_profilePrepareBaseAllocUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - baseAllocStartUs);
        g_vdp1VertexCapacity = vertexCount;
        g_vdp1IndexCapacity = static_cast<unsigned int>(genericIndexCount);
    }

    if (!g_vdp1Vertices ||
        !g_vdp1LightingVertices ||
        !g_vdp1Indices)
        return false;

    // Wires mode overlays the same 2x2 subdivision used by Full mode.
    // Keep this geometry in separate mapped buffers: the perimeter draw may
    // still be in flight when the subdivision overlay is submitted.
    const std::uint64_t wireStartUs = sceKernelGetProcessTimeWide();
    if (wireBuffersRequired && !reuseGeometry) {
        g_vdp1SubdivWireVertexCapacity =
            static_cast<unsigned int>(subdivWireVertexCount);
        g_vdp1SubdivWireIndexCapacity =
            static_cast<unsigned int>(subdivWireVertexCount);
        g_vdp1SubdivWireVertices =
            static_cast<azel::DebugColorVertex*>(
                probeGpuAlloc(
                    g_vdp1SubdivWireVertexCapacity *
                        sizeof(azel::DebugColorVertex),
                    SCE_GXM_MEMORY_ATTRIB_READ,
                    &g_vdp1SubdivWireVertexUid));
        g_vdp1SubdivWireIndices =
            static_cast<std::uint16_t*>(
                probeGpuAlloc(
                    g_vdp1SubdivWireIndexCapacity *
                        sizeof(std::uint16_t),
                    SCE_GXM_MEMORY_ATTRIB_READ,
                    &g_vdp1SubdivWireIndexUid));
        if (!g_vdp1SubdivWireVertices || !g_vdp1SubdivWireIndices) {
            g_vdp1SubdivWireVertexCapacity = 0u;
            g_vdp1SubdivWireIndexCapacity = 0u;
        } else {
            for (unsigned int i = 0;
                 i < g_vdp1SubdivWireIndexCapacity; ++i)
                g_vdp1SubdivWireIndices[i] =
                    static_cast<std::uint16_t>(i);
        }
    }
    g_profilePrepareWireUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - wireStartUs);

    if (model.texturesValid()) {
        if (!reuseTextures) {
            const std::uint64_t textureUploadStartUs =
                sceKernelGetProcessTimeWide();
            const bool uploaded = uploadVdp1Textures(model);
            g_profilePrepareTextureUploadUs = static_cast<unsigned int>(
                sceKernelGetProcessTimeWide() - textureUploadStartUs);
            if (!uploaded)
                return false;
        }
        if (!buildVdp1TexturedBuffers(model, reuseGeometry))
            return false;
        if (!reuseTextures)
            g_vdp1TextureDataDirty = false;
        g_vdp1TexturedReady = true;

        if (subdivBuffersRequired) {
            if (reuseGeometry) {
                // Positions, UVs, and topology are rewritten below into the
                // already-mapped capacity. No allocation is required.
            } else {
                g_vdp1SubdivVertexCapacity =
                    static_cast<unsigned int>(subdivVertexCount);
                g_vdp1SubdivIndexCapacity =
                    static_cast<unsigned int>(subdivIndexCount);
                const std::uint64_t subdivAllocStartUs =
                    sceKernelGetProcessTimeWide();
                g_vdp1SubdivVertices = static_cast<SubdivGouraudVertex*>(
                    probeGpuAlloc(
                        g_vdp1SubdivVertexCapacity *
                            sizeof(SubdivGouraudVertex),
                        SCE_GXM_MEMORY_ATTRIB_READ,
                        &g_vdp1SubdivVertexUid));
                g_vdp1SubdivIndices = static_cast<std::uint16_t*>(
                    probeGpuAlloc(
                        g_vdp1SubdivIndexCapacity * sizeof(std::uint16_t),
                        SCE_GXM_MEMORY_ATTRIB_READ,
                        &g_vdp1SubdivIndexUid));
                g_profilePrepareSubdivAllocUs = static_cast<unsigned int>(
                    sceKernelGetProcessTimeWide() - subdivAllocStartUs);
            }

            if (!g_vdp1SubdivVertices || !g_vdp1SubdivIndices) {
                g_vdp1SubdivVertexCapacity = 0u;
                g_vdp1SubdivIndexCapacity = 0u;
            } else {
                // Cache immutable subdivision topology, UVs and initial
                // positions once. Per frame, static town quads only need
                // shade updates; dynamic objects update position + shade.
                const std::uint64_t subdivBuildStartUs =
                    sceKernelGetProcessTimeWide();
                static const unsigned int cornerVertex[4] = {0u, 1u, 2u, 5u};
                static const unsigned int gridTris[24] = {
                    0,1,4, 0,4,3,
                    1,2,5, 1,5,4,
                    3,4,7, 3,7,6,
                    4,5,8, 4,8,7
                };
                g_vdp1SubdivQuadIndices.resize(subdivIndexCount);

                auto bilerp = [](
                    float a, float b, float c, float d,
                    float u, float v) {
                    const float top = a + (b - a) * u;
                    const float bottom = d + (c - d) * u;
                    return top + (bottom - top) * v;
                };

                for (unsigned int p = 0;
                     p < static_cast<unsigned int>(model.polygonCount); ++p) {
                    const auto& record = model.polygons[p];
                    const std::uint16_t textureIndex =
                        model.polygonTextureIndices[p];
                    if (textureIndex >= model.textureCount)
                        return false;
                    const auto& texture = model.textures[textureIndex];
                    if (!texture.width || !texture.height)
                        return false;

                    const float u0 =
                        0.5f / static_cast<float>(texture.width);
                    const float v0 =
                        0.5f / static_cast<float>(texture.height);
                    const float u1 =
                        (static_cast<float>(texture.width) - 0.5f) /
                        static_cast<float>(texture.width);
                    const float v1 =
                        (static_cast<float>(texture.height) - 0.5f) /
                        static_cast<float>(texture.height);
                    const float uv[4][2] = {
                        {u0,v0}, {u1,v0}, {u1,v1}, {u0,v1}
                    };
                    int order[4] = {0,1,2,3};
                    switch (record.textureFlip() & 3u) {
                    case 1:
                        order[0]=1; order[1]=0;
                        order[2]=3; order[3]=2;
                        break;
                    case 2:
                        order[0]=3; order[1]=2;
                        order[2]=1; order[3]=0;
                        break;
                    case 3:
                        order[0]=2; order[1]=3;
                        order[2]=0; order[3]=1;
                        break;
                    default:
                        break;
                    }

                    float cornerUv[4][2];
                    for (unsigned c = 0; c < 4u; ++c) {
                        cornerUv[c][0] = uv[order[c]][0];
                        cornerUv[c][1] = uv[order[c]][1];
                    }

                    const auto& a =
                        model.vertices[p * 6u + cornerVertex[0]];
                    const auto& b =
                        model.vertices[p * 6u + cornerVertex[1]];
                    const auto& c =
                        model.vertices[p * 6u + cornerVertex[2]];
                    const auto& d =
                        model.vertices[p * 6u + cornerVertex[3]];

                    const unsigned int baseVertex = p * 9u;
                    for (unsigned gy = 0; gy < 3u; ++gy) {
                        const float v = static_cast<float>(gy) * 0.5f;
                        for (unsigned gx = 0; gx < 3u; ++gx) {
                            const float u =
                                static_cast<float>(gx) * 0.5f;
                            auto& dst =
                                g_vdp1SubdivVertices[
                                    baseVertex + gy * 3u + gx];
                            dst.x = bilerp(a.x,b.x,c.x,d.x,u,v);
                            dst.y = bilerp(a.y,b.y,c.y,d.y,u,v);
                            dst.z = bilerp(a.z,b.z,c.z,d.z,u,v);
                            dst.u = bilerp(
                                cornerUv[0][0], cornerUv[1][0],
                                cornerUv[2][0], cornerUv[3][0],
                                u, v);
                            dst.v = bilerp(
                                cornerUv[0][1], cornerUv[1][1],
                                cornerUv[2][1], cornerUv[3][1],
                                u, v);
                            dst.shadeR = 0.0f;
                            dst.shadeG = 0.0f;
                            dst.shadeB = 0.0f;
                        }
                    }

                    const unsigned int quadIndexBase = p * 24u;
                    for (unsigned k = 0; k < 24u; ++k) {
                        g_vdp1SubdivQuadIndices[quadIndexBase + k] =
                            static_cast<std::uint16_t>(
                                baseVertex + gridTris[k]);
                    }
                }
                g_profilePrepareSubdivBuildUs = static_cast<unsigned int>(
                    sceKernelGetProcessTimeWide() - subdivBuildStartUs);
            }
        }
    } else {
        g_vdp1TexturedReady = false;
    }

    const std::uint64_t copyStartUs = sceKernelGetProcessTimeWide();
    std::memcpy(g_vdp1Vertices, model.vertices, vertexBytes);
    std::memcpy(g_vdp1LightingVertices, model.lightingVertices, vertexBytes);
    for (unsigned int i = 0; i < vertexCount; ++i)
        g_vdp1Indices[i] = static_cast<std::uint16_t>(i);
    g_profilePrepareCopyUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - copyStartUs);

    return true;
}

static void applyBasicWingAnimationFrame(unsigned int frameIndex)
{
    if (!g_basicWingCpuMesh.animationValid ||
        g_basicWingCpuMesh.animationFrames.empty())
        return;

    frameIndex %= static_cast<unsigned int>(
        g_basicWingCpuMesh.animationFrames.size());

    const auto& frame =
        g_basicWingCpuMesh.animationFrames[frameIndex];

    if (frame.vertices.size() !=
            g_basicWingCpuMesh.vertices.size() ||
        frame.lightingNormals.size() !=
            g_basicWingCpuMesh.lightingNormals.size())
        return;

    g_basicWingCpuMesh.vertices = frame.vertices;
    g_basicWingCpuMesh.lightingNormals =
        frame.lightingNormals;

    const unsigned int vertexCount =
        static_cast<unsigned int>(frame.vertices.size());

    for (unsigned int i = 0; i < vertexCount; ++i) {
        const auto& src = frame.vertices[i];

        if (g_vdp1Vertices) {
            g_vdp1Vertices[i].x = src.x;
            g_vdp1Vertices[i].y = src.y;
            g_vdp1Vertices[i].z = src.z;
        }

        if (g_vdp1LightingVertices) {
            g_vdp1LightingVertices[i].x = src.x;
            g_vdp1LightingVertices[i].y = src.y;
            g_vdp1LightingVertices[i].z = src.z;
        }

        if (g_vdp1TextureVertices) {
            g_vdp1TextureVertices[i].x = src.x;
            g_vdp1TextureVertices[i].y = src.y;
            g_vdp1TextureVertices[i].z = src.z;
        }

        if (g_vdp1GouraudVertices) {
            g_vdp1GouraudVertices[i].x = src.x;
            g_vdp1GouraudVertices[i].y = src.y;
            g_vdp1GouraudVertices[i].z = src.z;
        }
    }
}

static void advanceBasicWingAnimation()
{
    if (!g_basicWingCpuMesh.animationValid ||
        g_basicWingCpuMesh.animationFrames.empty())
        return;

    // Saturn animation is simulation-frame driven. If a frame takes longer
    // than 1/30 second, retain every pose and let playback slow down.
    const unsigned int frameCount = static_cast<unsigned int>(
        g_basicWingCpuMesh.animationFrames.size());
    g_basicWingAnimationFrame =
        (g_basicWingAnimationFrame + 1u) % frameCount;

    applyBasicWingAnimationFrame(
        g_basicWingAnimationFrame);
}

struct ViewerMat4
{
    float m[16];
};

static ViewerMat4 viewerIdentity()
{
    ViewerMat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static ViewerMat4 viewerMul(const ViewerMat4& a, const ViewerMat4& b)
{
    ViewerMat4 r{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            for (int k = 0; k < 4; ++k) {
                r.m[row * 4 + col] +=
                    a.m[row * 4 + k] * b.m[k * 4 + col];
            }
        }
    }
    return r;
}

static ViewerMat4 viewerRotationX(float angle)
{
    ViewerMat4 r = viewerIdentity();
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

static ViewerMat4 viewerRotationY(float angle)
{
    ViewerMat4 r = viewerIdentity();
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

static ViewerMat4 viewerTranslation(float x, float y, float z)
{
    ViewerMat4 r = viewerIdentity();
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
}

static void azelProjectionScales(
    float fullFovDegrees,
    unsigned int mode,
    float& outXScale,
    float& outYScale)
{
    constexpr float kPi = 3.14159265358979323846f;

    const float halfFovRadians =
        (fullFovDegrees * 0.5f) * kPi / 180.0f;
    const float cotHalf =
        1.0f / std::tan(halfFovRadians);

    float widthFactor = 352.0f / 320.0f;
    float heightFactor = 224.0f / 240.0f;

    if (mode == 1u) {
        widthFactor = 264.0f / 320.0f;
    } else if (mode == 2u) {
        widthFactor = 264.0f / 320.0f;
        heightFactor = 168.0f / 240.0f;
    }

    // Azel initVDP1Projection():
    //   r0 = (352 / 2) * cot(halfFov)
    //   widthScale  = r0 * widthFactor
    //   heightScale = r0 * heightFactor
    // 3dEngine_flush then normalizes those pixel-space projection scales by
    // half of the 352x224 VDP1 viewport.
    outXScale =
        cotHalf * widthFactor;
    outYScale =
        cotHalf *
        (176.0f / 112.0f) *
        heightFactor;

    // The Saturn 352x224 image is authored for a 4:3 display. The Vita
    // framebuffer is 16:9, so compress X into the centered 4:3 presentation
    // area while preserving Azel's original projection math.
    const float intendedAspect = 4.0f / 3.0f;
    const float vitaAspect =
        static_cast<float>(kWidth) /
        static_cast<float>(kHeight);
    outXScale *= intendedAspect / vitaAspect;
}

static ViewerMat4 buildAzelProjection(
    float fullFovDegrees,
    unsigned int mode,
    float nearZ,
    float farZ)
{
    float xScale = 1.0f;
    float yScale = 1.0f;
    azelProjectionScales(
        fullFovDegrees,
        mode,
        xScale,
        yScale);

    const float zScale =
        farZ / (farZ - nearZ);

    ViewerMat4 projection{};
    projection.m[0] = xScale;
    projection.m[5] = yScale;
    projection.m[10] = zScale;
    projection.m[11] = 1.0f;
    projection.m[14] = -nearZ * zScale;
    return projection;
}

static ViewerMat4 viewerLookAtLH(
    const float eye[3],
    const float target[3],
    const float upPoint[3])
{
    auto normalize = [](float v[3]) {
        const float lenSq =
            v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
        if (lenSq <= 0.0000001f)
            return;
        const float inv = 1.0f / std::sqrt(lenSq);
        v[0] *= inv; v[1] *= inv; v[2] *= inv;
    };
    auto cross = [](const float a[3], const float b[3], float out[3]) {
        out[0] = a[1]*b[2] - a[2]*b[1];
        out[1] = a[2]*b[0] - a[0]*b[2];
        out[2] = a[0]*b[1] - a[1]*b[0];
    };
    auto dot = [](const float a[3], const float b[3]) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };

    float z[3] = {
        target[0] - eye[0],
        target[1] - eye[1],
        target[2] - eye[2]
    };
    normalize(z);

    float up[3] = {
        upPoint[0] - eye[0],
        upPoint[1] - eye[1],
        upPoint[2] - eye[2]
    };
    normalize(up);

    // Keep the stable left-handed view basis here. Authentic Saturn
    // horizontal presentation is handled in buildAuthenticRoomWvp() by
    // mirroring projection X only. Changing this basis changes view-space
    // handedness and also affects the Azel visibility/LOD path.
    float x[3]{};
    cross(up, z, x);
    normalize(x);

    float y[3]{};
    cross(z, x, y);
    normalize(y);

    ViewerMat4 r = viewerIdentity();
    r.m[0] = x[0]; r.m[4] = x[1]; r.m[8]  = x[2];
    r.m[1] = y[0]; r.m[5] = y[1]; r.m[9]  = y[2];
    r.m[2] = z[0]; r.m[6] = z[1]; r.m[10] = z[2];
    r.m[12] = -dot(x, eye);
    r.m[13] = -dot(y, eye);
    r.m[14] = -dot(z, eye);
    return r;
}



static void transformTownEdgeVertices()
{
    if (!g_edgeIdleCpuReady || !g_townPlayerReady ||
        g_edgeIdleCpuMesh.vertices.empty())
        return;

    constexpr float kPi = 3.14159265358979323846f;
    const float a = g_townPlayerYaw + kPi; // sEdgeTask::Draw adds 180 degrees.
    const float c = std::cos(a);
    const float s = std::sin(a);
    const azel::BasicWingAnimationFrame* currentFrame = nullptr;
    const azel::BasicWingAnimationFrame* previousFrame = nullptr;

    if (g_townEdgeAnimation < g_edgeIdleCpuMesh.edgeAnimationClips.size()) {
        const auto& clip =
            g_edgeIdleCpuMesh.edgeAnimationClips[g_townEdgeAnimation];
        if (clip.valid && !clip.frames.empty())
            currentFrame = &clip.frames[
                g_townEdgeAnimationFrame % clip.frames.size()];
    }
    if (g_townEdgePreviousAnimation <
        g_edgeIdleCpuMesh.edgeAnimationClips.size()) {
        const auto& clip = g_edgeIdleCpuMesh.edgeAnimationClips[
            g_townEdgePreviousAnimation];
        if (clip.valid && !clip.frames.empty())
            previousFrame = &clip.frames[
                g_townEdgePreviousFrame % clip.frames.size()];
    }

    for (std::size_t i = 0; i < g_edgeIdleCpuMesh.vertices.size(); ++i) {
        const azel::DebugColorVertex* current =
            &g_edgeIdleCpuMesh.vertices[i];
        const azel::DebugColorVertex* previous = current;
        if (currentFrame && i < currentFrame->vertices.size())
            current = &currentFrame->vertices[i];
        if (previousFrame && i < previousFrame->vertices.size())
            previous = &previousFrame->vertices[i];
        azel::DebugColorVertex blended = *current;
        const float t = std::clamp(g_townEdgeTransition, 0.0f, 1.0f);
        blended.x = previous->x + (current->x - previous->x) * t;
        blended.y = previous->y + (current->y - previous->y) * t;
        blended.z = previous->z + (current->z - previous->z) * t;
        const auto& src = blended;
        const float x =
            src.x * c + src.z * s +
            g_townPlayerPosition[0];
        const float y =
            src.y + g_townPlayerPosition[1];
        const float z =
            -src.x * s + src.z * c +
            g_townPlayerPosition[2];

        const std::size_t dst = g_edgeFirstVertex + i;
        if (dst >= g_staticRoomCpuMesh.worldVertices.size())
            break;

        auto apply = [x,y,z](azel::DebugColorVertex& v) {
            v.x = x; v.y = y; v.z = z;
        };
        apply(g_staticRoomCpuMesh.worldVertices[dst]);
        apply(g_staticRoomCpuMesh.worldLightingVertices[dst]);
        apply(g_staticRoomCpuMesh.vertices[dst]);
        apply(g_staticRoomCpuMesh.lightingVertices[dst]);

        // If the combined room is resident, push only the moving position
        // fields. Texture coordinates/payload are unchanged.
        if (g_residentVdp1Model == ResidentVdp1Model::StaticRoomDiagnostic) {
            if (g_vdp1Vertices) apply(g_vdp1Vertices[dst]);
            if (g_vdp1LightingVertices) apply(g_vdp1LightingVertices[dst]);
            if (g_vdp1TextureVertices) {
                g_vdp1TextureVertices[dst].x = x;
                g_vdp1TextureVertices[dst].y = y;
                g_vdp1TextureVertices[dst].z = z;
            }
            if (g_vdp1GouraudVertices) {
                g_vdp1GouraudVertices[dst].x = x;
                g_vdp1GouraudVertices[dst].y = y;
                g_vdp1GouraudVertices[dst].z = z;
            }
        }
    }

    const float t = std::clamp(g_townEdgeTransition, 0.0f, 1.0f);
    for (std::size_t p = 0; p < g_edgeIdleCpuMesh.polygons; ++p) {
        const auto* current = p < g_edgeIdleCpuMesh.lightingNormals.size()
            ? &g_edgeIdleCpuMesh.lightingNormals[p] : nullptr;
        const auto* previous = current;
        if (currentFrame && p < currentFrame->lightingNormals.size())
            current = &currentFrame->lightingNormals[p];
        if (previousFrame && p < previousFrame->lightingNormals.size())
            previous = &previousFrame->lightingNormals[p];
        const std::size_t dst = g_edgeFirstPolygon + p;
        if (!current || !previous ||
            dst >= g_staticRoomCpuMesh.polygonRecords.size())
            continue;
        auto& record = g_staticRoomCpuMesh.polygonRecords[dst];
        const unsigned mode = (record.lightingControl >> 8) & 3u;
        const unsigned count = mode == 1u ? 1u : 4u;
        for (unsigned corner = 0; corner < count; ++corner) {
            float x = previous->corner[corner][0] +
                (current->corner[corner][0] - previous->corner[corner][0]) * t;
            float y = previous->corner[corner][1] +
                (current->corner[corner][1] - previous->corner[corner][1]) * t;
            float z = previous->corner[corner][2] +
                (current->corner[corner][2] - previous->corner[corner][2]) * t;
            const float length = std::sqrt(x*x + y*y + z*z);
            if (length > 0.000001f) {
                x /= length; y /= length; z /= length;
            }
            const float worldX = x*c + z*s;
            const float worldZ = -x*s + z*c;
            record.lighting[corner].normal[0] =
                static_cast<std::int16_t>(std::lround(worldX * 4096.0f));
            record.lighting[corner].normal[1] =
                static_cast<std::int16_t>(std::lround(y * 4096.0f));
            record.lighting[corner].normal[2] =
                static_cast<std::int16_t>(std::lround(worldZ * 4096.0f));
        }
    }
}

static bool decodeLiveVdp1Texture(
    const azel::SaturnPolygonRecord& record,
    azel::DecodedMode1Texture& out)
{
    const unsigned commandType =
        static_cast<unsigned>(record.cmdCtrl) & 0x000Fu;
    const unsigned width = record.textureWidth();
    const unsigned height = record.textureHeight();
    const unsigned mode = record.colorMode();

    out = {};
    out.cmdPmod = record.cmdPmod;
    out.cmdColr = record.cmdColr;
    out.cmdSrca = record.cmdSrca;
    out.cmdSize = record.cmdSize;

    // VDP1 command type 4 is an untextured polygon. CMDSIZE is therefore
    // legitimately zero; CMDCOLR supplies the flat RGB555 color. Represent
    // it as a 1x1 material so it can stay on the same batched GXM path.
    if (commandType == 4u) {
        if ((record.cmdColr & 0x8000u) == 0u)
            return false;

        out.width = 1u;
        out.height = 1u;
        out.rgba.assign(1u, vdp2Rgb555ToAbgr(record.cmdColr));

        static bool loggedSolidPolygon = false;
        if (!loggedSolidPolygon) {
            logging::writef(
                "[SceneRender] native flat polygon "
                "CTRL=%04X PMOD=%04X COLR=%04X\n",
                static_cast<unsigned>(record.cmdCtrl),
                static_cast<unsigned>(record.cmdPmod),
                static_cast<unsigned>(record.cmdColr));
            loggedSolidPolygon = true;
        }
        return true;
    }

    if (!width || !height)
        return false;

    const unsigned textureAddress =
        static_cast<unsigned>(record.cmdSrca) << 3;
    const unsigned char* const src =
        getVdp1Pointer(0x25C00000u + textureAddress);
    if (!src)
        return false;

    out.width = width;
    out.height = height;
    out.rgba.assign(static_cast<std::size_t>(width) * height, 0u);

    const bool spd = (record.cmdPmod & 0x40u) != 0u;
    const bool endDisabled = (record.cmdPmod & 0x80u) != 0u;
    const bool endMode = (record.cmdPmod & 0x20u) == 0u;

    auto cramColor = [](unsigned index) -> std::uint32_t {
        const unsigned byte = (index * 2u) & 0x0FFFu;
        const std::uint16_t c = readVdp2Be16(g_vdp2Cram, byte);
        return c ? vdp2Rgb555ToAbgr(c) : 0u;
    };

    auto decodeDot4 = [&](auto resolveColor) {
        unsigned pixel = 0u;
        for (unsigned y = 0; y < height; ++y) {
            unsigned endCount = 0u;
            for (unsigned x = 0; x < width; ++x, ++pixel) {
                const std::uint8_t packed =
                    src[(x + y * width) / 2u];
                const unsigned dot =
                    (x & 1u) ? (packed & 0x0Fu) : (packed >> 4);
                if (endMode && endCount >= 2u)
                    continue;
                if (dot == 0u && !spd)
                    continue;
                if (dot == 0x0Fu && !endDisabled) {
                    ++endCount;
                    continue;
                }
                out.rgba[pixel] = resolveColor(dot);
            }
        }
    };

    switch (mode) {
    case 0u: {
        const unsigned bank =
            static_cast<unsigned>(record.cmdColr) & 0x07F0u;
        decodeDot4([&](unsigned dot) {
            return cramColor(bank | dot);
        });
        return true;
    }

    case 1u: {
        const unsigned lutAddress =
            static_cast<unsigned>(record.cmdColr) << 3;
        const unsigned char* const lut =
            getVdp1Pointer(0x25C00000u + lutAddress);
        if (!lut)
            return false;

        decodeDot4([&](unsigned dot) {
            const std::uint16_t entry =
                static_cast<std::uint16_t>(
                    (static_cast<unsigned>(lut[dot * 2u]) << 8) |
                    static_cast<unsigned>(lut[dot * 2u + 1u]));
            if (entry & 0x8000u)
                return vdp2Rgb555ToAbgr(entry);
            return entry ? cramColor(entry & 0x07FFu) : 0u;
        });
        return true;
    }

    case 5u: {
        for (unsigned p = 0; p < width * height; ++p) {
            const std::uint16_t color =
                static_cast<std::uint16_t>(
                    (static_cast<unsigned>(src[p * 2u]) << 8) |
                    static_cast<unsigned>(src[p * 2u + 1u]));
            if (color & 0x8000u)
                out.rgba[p] = vdp2Rgb555ToAbgr(color);
        }
        return true;
    }

    default:
        return false;
    }
}

static std::uint16_t liveTownTextureIndex(
    const azel::SaturnPolygonRecord& record)
{
    for (std::size_t i = 0;
         i < g_staticRoomCpuMesh.decodedTextureData.size(); ++i) {
        const auto& texture = g_staticRoomCpuMesh.decodedTextureData[i];
        if (record.cmdPmod == texture.cmdPmod &&
            record.cmdColr == texture.cmdColr &&
            record.cmdSrca == texture.cmdSrca &&
            record.cmdSize == texture.cmdSize)
            return static_cast<std::uint16_t>(i);
    }

    // Authentic boot has already populated VDP1 memory. Decode the live
    // descriptor directly from that Saturn address space rather than relying
    // on the historical direct-boot town bundle/overlay registry.
    azel::DecodedMode1Texture decoded{};
    if (decodeLiveVdp1Texture(record, decoded)) {
        const std::size_t next =
            g_staticRoomCpuMesh.decodedTextureData.size();
        if (next < 0xFFFFu) {
            g_staticRoomCpuMesh.decodedTextureData.push_back(
                std::move(decoded));
            return static_cast<std::uint16_t>(next);
        }
    }

    static unsigned int unresolvedLogged = 0u;
    if (unresolvedLogged < 16u) {
        platform::logging::writef(
            "[SceneRender] unresolved live material "
            "CTRL=%04X PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X mode=%u\n",
            static_cast<unsigned>(record.cmdCtrl),
            static_cast<unsigned>(record.cmdPmod),
            static_cast<unsigned>(record.cmdColr),
            static_cast<unsigned>(record.cmdSrca),
            static_cast<unsigned>(record.cmdSize),
            record.colorMode());
        ++unresolvedLogged;
    }
    return 0xFFFFu;
}

static Vdp1ModelSource liveTownVdp1Source()
{
    Vdp1ModelSource source{};
    source.vertices = g_liveTownCpuMesh.vertices.data();
    source.lightingVertices = g_liveTownCpuMesh.lightingVertices.data();
    source.vertexCount = g_liveTownCpuMesh.vertices.size();
    source.polygons = g_liveTownCpuMesh.polygonRecords.data();
    source.gouraud555 = g_liveTownCpuMesh.gouraud555.data();
    source.polygonCount = g_liveTownCpuMesh.polygonRecords.size();
    source.textures = g_staticRoomCpuMesh.decodedTextureData.data();
    source.textureCount = g_staticRoomCpuMesh.decodedTextureData.size();
    source.polygonTextureIndices =
        g_liveTownCpuMesh.polygonTextureIndices.data();
    source.polygonTextureIndexCount =
        g_liveTownCpuMesh.polygonTextureIndices.size();
    return source;
}

static const std::vector<std::uint16_t>* resolvedLiveTownMaterialIndices(
    sProcessed3dModel* modelIdentity,
    const azel_bridge::LiveVdp1Model& model)
{
    if (!modelIdentity)
        return nullptr;

    for (auto& cached : g_liveTownMaterialCache) {
        if (cached.model == modelIdentity &&
            cached.textureIndices.size() == model.polygons.size())
            return &cached.textureIndices;
    }

    const std::uint64_t resolveStartUs = sceKernelGetProcessTimeWide();
    LiveTownResolvedMaterialCache cached{};
    cached.model = modelIdentity;
    cached.textureIndices.reserve(model.polygons.size());
    for (const auto& record : model.polygons)
        cached.textureIndices.push_back(liveTownTextureIndex(record));
    g_profileObjectMaterialResolveUs += static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - resolveStartUs);
    ++g_profileObjectMaterialCacheMisses;

    g_liveTownMaterialCache.push_back(std::move(cached));
    return &g_liveTownMaterialCache.back().textureIndices;
}

static void appendLiveTownModel(
    const azel_bridge::LiveVdp1Model& model,
    const azel_bridge::SubmissionState& state,
    const std::uint16_t* resolvedTextureIndices = nullptr,
    std::size_t resolvedTextureIndexCount = 0)
{
    if (!state.hasModelMatrix)
        return;

    const float invFixed = 1.0f / 65536.0f;
    float m[12]{};
    for (unsigned i = 0; i < 12; ++i)
        m[i] = state.modelMatrix[i] * invFixed;

    float billboardX[3]{}, billboardY[3]{}, billboardZ[3]{};
    if (state.billboard) {
        billboardZ[0] = g_townCameraTarget[0] - g_townCameraPosition[0];
        billboardZ[1] = g_townCameraTarget[1] - g_townCameraPosition[1];
        billboardZ[2] = g_townCameraTarget[2] - g_townCameraPosition[2];
        const float zl = std::sqrt(
            billboardZ[0]*billboardZ[0] + billboardZ[1]*billboardZ[1] +
            billboardZ[2]*billboardZ[2]);
        if (zl > 0.000001f)
            for (float& v : billboardZ) v /= zl;
        float up[3] = {
            g_townCameraUp[0] - g_townCameraPosition[0],
            g_townCameraUp[1] - g_townCameraPosition[1],
            g_townCameraUp[2] - g_townCameraPosition[2]};
        const float ul = std::sqrt(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
        if (ul > 0.000001f)
            for (float& v : up) v /= ul;
        billboardX[0] = up[1]*billboardZ[2] - up[2]*billboardZ[1];
        billboardX[1] = up[2]*billboardZ[0] - up[0]*billboardZ[2];
        billboardX[2] = up[0]*billboardZ[1] - up[1]*billboardZ[0];
        const float xl = std::sqrt(
            billboardX[0]*billboardX[0] + billboardX[1]*billboardX[1] +
            billboardX[2]*billboardX[2]);
        if (xl > 0.000001f)
            for (float& v : billboardX) v /= xl;
        billboardY[0] = billboardZ[1]*billboardX[2] - billboardZ[2]*billboardX[1];
        billboardY[1] = billboardZ[2]*billboardX[0] - billboardZ[0]*billboardX[2];
        billboardY[2] = billboardZ[0]*billboardX[1] - billboardZ[1]*billboardX[0];
    }

    const std::size_t polygonBase = g_liveTownCpuMesh.polygonRecords.size();
    for (const auto& source : model.vertices) {
        azel::DebugColorVertex v = source;
        if (state.billboard) {
            v.x = m[3] + source.x*billboardX[0] + source.y*billboardY[0] +
                source.z*billboardZ[0];
            v.y = m[7] + source.x*billboardX[1] + source.y*billboardY[1] +
                source.z*billboardZ[1];
            v.z = m[11] + source.x*billboardX[2] + source.y*billboardY[2] +
                source.z*billboardZ[2];
        } else {
            v.x = source.x*m[0] + source.y*m[1] + source.z*m[2] + m[3];
            v.y = source.x*m[4] + source.y*m[5] + source.z*m[6] + m[7];
            v.z = source.x*m[8] + source.y*m[9] + source.z*m[10] + m[11];
        }
        g_liveTownCpuMesh.vertices.push_back(v);
        g_liveTownCpuMesh.lightingVertices.push_back(v);
    }

    for (std::size_t p = 0; p < model.polygons.size(); ++p) {
        auto record = model.polygons[p];

        if (record.cmdPmod & 0x0100u) {
            static unsigned int liveMeshTraceBudget = 48u;
            if (liveMeshTraceBudget != 0u) {
                const std::uint16_t resolvedIndex =
                    resolvedTextureIndices &&
                    p < resolvedTextureIndexCount
                        ? resolvedTextureIndices[p]
                        : 0xFFFFu;
                logging::writef(
                    "[PresentationTrace][LiveMesh] p=%u dynamic=%u billboard=%u "
                    "CTRL=%04X PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X "
                    "resolved=%u\n",
                    static_cast<unsigned int>(p),
                    state.dynamic ? 1u : 0u,
                    state.billboard ? 1u : 0u,
                    static_cast<unsigned int>(record.cmdCtrl),
                    static_cast<unsigned int>(record.cmdPmod),
                    static_cast<unsigned int>(record.cmdColr),
                    static_cast<unsigned int>(record.cmdSrca),
                    static_cast<unsigned int>(record.cmdSize),
                    static_cast<unsigned int>(resolvedIndex));
                --liveMeshTraceBudget;
            }
        }
        for (unsigned n = 0; n < record.lightingCount; ++n) {
            const float x = record.lighting[n].normal[0] / 4096.0f;
            const float y = record.lighting[n].normal[1] / 4096.0f;
            const float z = record.lighting[n].normal[2] / 4096.0f;
            float nx, ny, nz;
            if (state.billboard) {
                nx = x*billboardX[0] + y*billboardY[0] + z*billboardZ[0];
                ny = x*billboardX[1] + y*billboardY[1] + z*billboardZ[1];
                nz = x*billboardX[2] + y*billboardY[2] + z*billboardZ[2];
            } else {
                nx = x*m[0] + y*m[1] + z*m[2];
                ny = x*m[4] + y*m[5] + z*m[6];
                nz = x*m[8] + y*m[9] + z*m[10];
            }
            record.lighting[n].normal[0] = static_cast<std::int16_t>(
                std::lround(std::clamp(nx, -1.0f, 1.0f) * 4096.0f));
            record.lighting[n].normal[1] = static_cast<std::int16_t>(
                std::lround(std::clamp(ny, -1.0f, 1.0f) * 4096.0f));
            record.lighting[n].normal[2] = static_cast<std::int16_t>(
                std::lround(std::clamp(nz, -1.0f, 1.0f) * 4096.0f));
        }
        record.model = static_cast<unsigned int>(polygonBase);
        g_liveTownCpuMesh.polygonRecords.push_back(record);
        const std::uint16_t textureIndex =
            resolvedTextureIndices && p < resolvedTextureIndexCount
                ? resolvedTextureIndices[p]
                : liveTownTextureIndex(record);
        g_liveTownCpuMesh.polygonTextureIndices.push_back(textureIndex);
        g_liveTownCpuMesh.gouraud555.push_back({});

        LivePolygonLightState polygonLight{};
        if (state.hasLight) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                polygonLight.vector[axis] = state.lightVector[axis];
                polygonLight.color[axis] = state.lightColor[axis];
                polygonLight.falloff[axis] = state.lightFalloff[axis];
            }
            polygonLight.valid = true;
        }
        g_liveTownPolygonLights.push_back(polygonLight);
    }

    // Preserve the exact positions of Azel-authored VDP1 mesh commands in the
    // flattened live-town stream. Adjacent mesh polygons from the same model
    // are coalesced into ranges, but no scene/object identity is inferred.
    std::size_t runStart = 0u;
    std::size_t runCount = 0u;
    for (std::size_t p = 0; p < model.polygons.size(); ++p) {
        const bool mesh = (model.polygons[p].cmdPmod & 0x0100u) != 0u;
        if (mesh) {
            if (runCount == 0u)
                runStart = p;
            ++runCount;
        } else if (runCount != 0u) {
            g_liveTownMeshRanges.push_back(
                {polygonBase + runStart, runCount});
            runCount = 0u;
        }
    }
    if (runCount != 0u)
        g_liveTownMeshRanges.push_back(
            {polygonBase + runStart, runCount});
}

static void appendLiveTownEdge()
{
    g_liveTownShadowFirstPolygon = 0u;
    g_liveTownShadowPolygonCount = 0u;
    g_liveTownEdgeFirstPolygon = 0u;
    g_liveTownEdgePolygonCount = 0u;

    g_profileEdgeCopyUs = 0u;
    g_profileEdgeAnimUs = 0u;
    g_profileEdgeAppendUs = 0u;
    if (!g_edgeIdleCpuReady || !g_townPlayerReady)
        return;

    const std::uint64_t tCopy = sceKernelGetProcessTimeWide();
    azel_bridge::LiveVdp1Model edge{};
    edge.vertices = g_edgeIdleCpuMesh.vertices;
    edge.lightingVertices = g_edgeIdleCpuMesh.lightingVertices;
    edge.polygons = g_edgeIdleCpuMesh.polygonRecords;
    edge.gouraud555.resize(edge.polygons.size());
    g_profileEdgeCopyUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tCopy);

    const std::uint64_t tAnim = sceKernelGetProcessTimeWide();
    const azel::BasicWingAnimationFrame* current = nullptr;
    const azel::BasicWingAnimationFrame* previous = nullptr;
    if (g_townEdgeAnimation < g_edgeIdleCpuMesh.edgeAnimationClips.size()) {
        const auto& clip = g_edgeIdleCpuMesh.edgeAnimationClips[g_townEdgeAnimation];
        if (clip.valid && !clip.frames.empty())
            current = &clip.frames[g_townEdgeAnimationFrame % clip.frames.size()];
    }
    if (g_townEdgePreviousAnimation < g_edgeIdleCpuMesh.edgeAnimationClips.size()) {
        const auto& clip = g_edgeIdleCpuMesh.edgeAnimationClips[g_townEdgePreviousAnimation];
        if (clip.valid && !clip.frames.empty())
            previous = &clip.frames[g_townEdgePreviousFrame % clip.frames.size()];
    }
    const float blend = std::clamp(g_townEdgeTransition, 0.0f, 1.0f);
    if (current && previous && current->vertices.size() == edge.vertices.size() &&
        previous->vertices.size() == edge.vertices.size()) {
        for (std::size_t i = 0; i < edge.vertices.size(); ++i) {
            edge.vertices[i] = previous->vertices[i];
            edge.vertices[i].x += (current->vertices[i].x - edge.vertices[i].x) * blend;
            edge.vertices[i].y += (current->vertices[i].y - edge.vertices[i].y) * blend;
            edge.vertices[i].z += (current->vertices[i].z - edge.vertices[i].z) * blend;
        }
    } else if (current && current->vertices.size() == edge.vertices.size()) {
        edge.vertices = current->vertices;
    }
    if (current && previous &&
        current->lightingNormals.size() == edge.polygons.size() &&
        previous->lightingNormals.size() == edge.polygons.size()) {
        for (std::size_t p = 0; p < edge.polygons.size(); ++p) {
            auto& record = edge.polygons[p];
            const unsigned mode = (record.lightingControl >> 8) & 3u;
            const unsigned count = mode == 1u ? 1u : record.lightingCount;
            for (unsigned corner = 0; corner < count; ++corner) {
                float normal[3]{};
                for (unsigned axis = 0; axis < 3; ++axis) {
                    const float from = previous->lightingNormals[p].corner[corner][axis];
                    const float to = current->lightingNormals[p].corner[corner][axis];
                    normal[axis] = from + (to - from) * blend;
                }
                const float nl = std::sqrt(
                    normal[0]*normal[0] + normal[1]*normal[1] +
                    normal[2]*normal[2]);
                if (nl > 0.000001f)
                    for (float& v : normal) v /= nl;
                for (unsigned axis = 0; axis < 3; ++axis)
                    record.lighting[corner].normal[axis] =
                        static_cast<std::int16_t>(std::lround(
                            std::clamp(normal[axis], -1.0f, 1.0f) * 4096.0f));
            }
        }
    }

    g_profileEdgeAnimUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tAnim);
    const std::uint64_t tAppend = sceKernelGetProcessTimeWide();

    azel_bridge::SubmissionState state{};
    constexpr float kPi = 3.14159265358979323846f;
    const float a = g_townPlayerYaw + kPi;
    const float c = std::cos(a), s = std::sin(a);
    state.modelMatrix[0] = static_cast<std::int32_t>(std::lround(c * 65536.0f));
    state.modelMatrix[2] = static_cast<std::int32_t>(std::lround(s * 65536.0f));
    state.modelMatrix[5] = 0x10000;
    state.modelMatrix[8] = static_cast<std::int32_t>(std::lround(-s * 65536.0f));
    state.modelMatrix[10] = static_cast<std::int32_t>(std::lround(c * 65536.0f));
    state.modelMatrix[3] = static_cast<std::int32_t>(std::lround(g_townPlayerPosition[0] * 65536.0f));
    state.modelMatrix[7] = static_cast<std::int32_t>(std::lround(g_townPlayerPosition[1] * 65536.0f));
    state.modelMatrix[11] = static_cast<std::int32_t>(std::lround(g_townPlayerPosition[2] * 65536.0f));
    state.hasModelMatrix = true;

    // Edge's VDP1 texture mapping was already resolved when the static room
    // asset set was assembled. Reuse those indices instead of performing a
    // linear search across every decoded room texture for every Edge polygon
    // on every frame.
    const std::uint16_t* edgeTextureIndices = nullptr;
    std::size_t edgeTextureIndexCount = 0;
    if (g_edgeFirstPolygon < g_staticRoomCpuMesh.polygonTextureIndices.size()) {
        edgeTextureIndices =
            g_staticRoomCpuMesh.polygonTextureIndices.data() +
            g_edgeFirstPolygon;
        edgeTextureIndexCount = std::min<std::size_t>(
            edge.polygons.size(),
            g_staticRoomCpuMesh.polygonTextureIndices.size() -
                g_edgeFirstPolygon);
    }

    // Original sEdgeTask::Draw submits the COMMON3 mesh shadow under the
    // exact same Edge transform immediately before the animated actor.
    if (g_edgeShadowCpuReady &&
        !g_edgeShadowTownTextureIndices.empty()) {
        azel_bridge::LiveVdp1Model shadow{};
        shadow.vertices = g_edgeShadowCpuMesh.vertices;
        shadow.lightingVertices = g_edgeShadowCpuMesh.lightingVertices;
        shadow.polygons = g_edgeShadowCpuMesh.polygonRecords;
        shadow.gouraud555.resize(shadow.polygons.size());
        azel_bridge::SubmissionState shadowState = state;
        // Keep Azel's original transform. VDP1 mesh visibility is reproduced
        // in the GXM draw state below rather than by moving the geometry.
        g_liveTownShadowFirstPolygon =
            g_liveTownCpuMesh.polygonRecords.size();
        appendLiveTownModel(
            shadow,
            shadowState,
            g_edgeShadowTownTextureIndices.data(),
            g_edgeShadowTownTextureIndices.size());
        g_liveTownShadowPolygonCount =
            g_liveTownCpuMesh.polygonRecords.size() -
            g_liveTownShadowFirstPolygon;
    }

    g_liveTownEdgeFirstPolygon =
        g_liveTownCpuMesh.polygonRecords.size();
    appendLiveTownModel(
        edge, state, edgeTextureIndices, edgeTextureIndexCount);
    g_liveTownEdgePolygonCount =
        g_liveTownCpuMesh.polygonRecords.size() -
        g_liveTownEdgeFirstPolygon;
    g_profileEdgeAppendUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tAppend);
}

static void refreshLiveTownStaticLighting()
{
    std::size_t polygonBase = 0u;

    for (const auto& submission : azel_bridge::published_submissions()) {
        if (submission.state.dynamic ||
            submission.adaptedModelIndex < 0)
            continue;

        const auto* model = azel_bridge::published_adapted_model(
            static_cast<std::uint32_t>(submission.adaptedModelIndex));
        if (!model)
            continue;

        const std::size_t polygonCount = model->polygons.size();
        if (polygonBase + polygonCount > g_liveTownStaticPolygonCount ||
            polygonBase + polygonCount > g_liveTownPolygonLights.size())
            break;

        LivePolygonLightState light{};
        if (submission.state.hasLight) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                light.vector[axis] = submission.state.lightVector[axis];
                light.color[axis] = submission.state.lightColor[axis];
                light.falloff[axis] = submission.state.lightFalloff[axis];
            }
            light.valid = true;
        }

        for (std::size_t p = 0; p < polygonCount; ++p)
            g_liveTownPolygonLights[polygonBase + p] = light;

        polygonBase += polygonCount;
    }
}

static bool buildLiveTownFrame()
{
    const std::uint64_t buildStartUs = sceKernelGetProcessTimeWide();
    const unsigned int decodedTexturesBefore =
        static_cast<unsigned int>(
            g_staticRoomCpuMesh.decodedTextureData.size());
    const unsigned int modelCacheMisses =
        azel_bridge::published_model_cache_misses();

    g_profileBuildScanUs = 0u;
    g_profileBuildCacheUs = 0u;
    g_profileBuildEdgeUs = 0u;
    g_profileBuildValidateUs = 0u;
    g_profileBuildUploadUs = 0u;
    g_profilePrepareReleaseUs = 0u;
    g_profilePrepareBaseAllocUs = 0u;
    g_profilePrepareWireUs = 0u;
    g_profilePrepareTextureUploadUs = 0u;
    g_profilePrepareTexturedAllocUs = 0u;
    g_profilePrepareTexturedBuildUs = 0u;
    g_profilePrepareSubdivAllocUs = 0u;
    g_profilePrepareSubdivBuildUs = 0u;
    g_profilePrepareCopyUs = 0u;
    g_profilePrepareReusedTextures = false;
    g_profilePrepareReusedGeometry = false;
    g_profileObjectAppendUs = 0u;
    g_profileObjectMaterialResolveUs = 0u;
    g_profileObjectMaterialCacheMisses = 0u;
    const std::uint64_t tScan = sceKernelGetProcessTimeWide();
    std::uint64_t staticSignature = 1469598103934665603ull;
    bool hasBillboards = false;
    g_liveTownSubmissionCount = 0u;
    g_liveTownMeshRanges.clear();
    g_liveTownStaticSubmissionCount = 0u;
    g_liveTownBillboardSubmissionCount = 0u;
    g_liveTownStaticSubmittedPolygons = 0u;
    g_liveTownBillboardSubmittedPolygons = 0u;

    for (const auto& submission : azel_bridge::published_submissions()) {
        if (submission.adaptedModelIndex < 0)
            continue;
        const auto* model = azel_bridge::published_adapted_model(
            static_cast<std::uint32_t>(submission.adaptedModelIndex));
        if (!model) continue;

        ++g_liveTownSubmissionCount;
        if (submission.state.billboard) {
            ++g_liveTownBillboardSubmissionCount;
            g_liveTownBillboardSubmittedPolygons +=
                static_cast<unsigned int>(model->polygons.size());
        } else if (!submission.state.dynamic) {
            ++g_liveTownStaticSubmissionCount;
            g_liveTownStaticSubmittedPolygons +=
                static_cast<unsigned int>(model->polygons.size());
        }

        if (!submission.state.dynamic) {
            // Geometry/material identity only. Lighting is renderer state and
            // must not invalidate/rebuild the static world mesh.
            staticSignature ^= submission.modelTableOffset;
            staticSignature *= 1099511628211ull;
            staticSignature ^= model->polygons.size();
            staticSignature *= 1099511628211ull;
        }
        hasBillboards = hasBillboards || submission.state.billboard;
    }
    g_liveTownHasBillboards = hasBillboards;
    g_profileBuildScanUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tScan);

    const std::uint64_t tCache = sceKernelGetProcessTimeWide();
    g_liveTownStaticRebuilt = hasBillboards ||
        staticSignature != g_liveTownStaticSignature;
    if (g_liveTownStaticRebuilt) {
        g_liveTownCpuMesh.vertices.clear();
        g_liveTownCpuMesh.lightingVertices.clear();
        g_liveTownCpuMesh.polygonRecords.clear();
        g_liveTownCpuMesh.gouraud555.clear();
        g_liveTownCpuMesh.polygonTextureIndices.clear();
        g_liveTownPolygonLights.clear();
        for (const auto& submission : azel_bridge::published_submissions()) {
            if (submission.state.dynamic ||
                submission.adaptedModelIndex < 0)
                continue;
            const auto* model = azel_bridge::published_adapted_model(
                static_cast<std::uint32_t>(submission.adaptedModelIndex));
            if (model) appendLiveTownModel(*model, submission.state);
        }
        g_liveTownStaticSignature = staticSignature;
        g_liveTownStaticVertexCount = g_liveTownCpuMesh.vertices.size();
        g_liveTownStaticPolygonCount = g_liveTownCpuMesh.polygonRecords.size();
    } else {
        g_liveTownCpuMesh.vertices.resize(g_liveTownStaticVertexCount);
        g_liveTownCpuMesh.lightingVertices.resize(g_liveTownStaticVertexCount);
        g_liveTownCpuMesh.polygonRecords.resize(g_liveTownStaticPolygonCount);
        g_liveTownCpuMesh.gouraud555.resize(g_liveTownStaticPolygonCount);
        g_liveTownCpuMesh.polygonTextureIndices.resize(
            g_liveTownStaticPolygonCount);
        g_liveTownPolygonLights.resize(g_liveTownStaticPolygonCount);
    }
    g_profileBuildCacheUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tCache);

    // Geometry/materials are cached independently from lighting. Refresh the
    // current Azel light payload for the cached static polygon ranges every
    // frame without rebuilding or re-uploading the static mesh.
    refreshLiveTownStaticLighting();

    // Task-owned town objects retain their native per-frame transform
    // lifecycle. Their model/material mapping is immutable, though, so cache
    // polygon->town-texture indices by native model identity exactly as Edge
    // reuses its already-resolved mapping. Only transforms/normals are rebuilt
    // per instance each frame.
    const std::uint64_t tObjects = sceKernelGetProcessTimeWide();
    for (const auto& submission : azel_bridge::published_submissions()) {
        if (!submission.state.dynamic ||
            submission.adaptedModelIndex < 0)
            continue;
        const auto* model = azel_bridge::published_adapted_model(
            static_cast<std::uint32_t>(submission.adaptedModelIndex));
        if (!model)
            continue;

        const auto* resolved = resolvedLiveTownMaterialIndices(
            submission.model, *model);
        appendLiveTownModel(
            *model,
            submission.state,
            resolved && !resolved->empty() ? resolved->data() : nullptr,
            resolved ? resolved->size() : 0u);
    }
    g_profileObjectAppendUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tObjects);

    const std::uint64_t tEdge = sceKernelGetProcessTimeWide();
    // Town mode owns Edge. Field mode publishes its dragon/rider hierarchy
    // through Azel's normal addObjectToDrawList() path, so never inject the
    // town actor into a field frame.
    if (g_sceneGameMode == 1u)
        appendLiveTownEdge();
    else {
        g_liveTownShadowFirstPolygon = 0u;
        g_liveTownShadowPolygonCount = 0u;
        g_liveTownEdgeFirstPolygon = 0u;
        g_liveTownEdgePolygonCount = 0u;
    }
    g_profileBuildEdgeUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tEdge);

    const std::uint64_t tValidate = sceKernelGetProcessTimeWide();
    std::uint64_t signature = staticSignature;
    signature ^= g_liveTownCpuMesh.polygonRecords.size();
    signature *= 1099511628211ull;
    signature ^= g_staticRoomCpuMesh.decodedTextureData.size();
    signature *= 1099511628211ull;
    g_liveTownCpuMesh.polygons = static_cast<unsigned int>(
        g_liveTownCpuMesh.polygonRecords.size());
    if (g_liveTownCpuMesh.vertices.empty() ||
        g_liveTownCpuMesh.vertices.size() > 65535u ||
        g_liveTownCpuMesh.polygonTextureIndices.size() !=
            g_liveTownCpuMesh.polygonRecords.size() ||
        g_liveTownPolygonLights.size() !=
            g_liveTownCpuMesh.polygonRecords.size()) {
        static bool loggedInvalidGeometry = false;
        if (!loggedInvalidGeometry) {
            logging::writef(
                "[SceneRender] live frame validation failed "
                "verts=%u polys=%u texIndices=%u submissions=%u atlas=%u\n",
                static_cast<unsigned>(g_liveTownCpuMesh.vertices.size()),
                static_cast<unsigned>(g_liveTownCpuMesh.polygonRecords.size()),
                static_cast<unsigned>(
                    g_liveTownCpuMesh.polygonTextureIndices.size()),
                g_liveTownSubmissionCount,
                static_cast<unsigned>(
                    g_staticRoomCpuMesh.decodedTextureData.size()));
            loggedInvalidGeometry = true;
        }
        return false;
    }

    for (std::size_t i = 0;
         i < g_liveTownCpuMesh.polygonTextureIndices.size(); ++i) {
        if (g_liveTownCpuMesh.polygonTextureIndices[i] == 0xFFFFu) {
            static bool loggedInvalidMaterial = false;
            if (!loggedInvalidMaterial) {
                logging::writef(
                    "[SceneRender] live frame rejected unresolved material "
                    "poly=%u polys=%u atlas=%u submissions=%u\n",
                    static_cast<unsigned>(i),
                    static_cast<unsigned>(
                        g_liveTownCpuMesh.polygonRecords.size()),
                    static_cast<unsigned>(
                        g_staticRoomCpuMesh.decodedTextureData.size()),
                    g_liveTownSubmissionCount);
                loggedInvalidMaterial = true;
            }
            return false;
        }
    }
    g_profileBuildValidateUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tValidate);

    const std::uint64_t tUpload = sceKernelGetProcessTimeWide();
    const bool changed = !g_liveTownPrepared ||
        g_residentVdp1Model != ResidentVdp1Model::LiveTown ||
        signature != g_liveTownSignature;
    g_liveTownSignature = signature;
    if (changed) {
        if (!prepare_vdp1_model(
                liveTownVdp1Source(),
                g_sceneGameMode == 3u))
            return false;
        g_residentVdp1Model = ResidentVdp1Model::LiveTown;
        g_liveTownPrepared = true;
    } else {
        const std::size_t firstDynamicVertex = g_liveTownStaticRebuilt
            ? 0u : g_liveTownStaticVertexCount;
        for (std::size_t i = firstDynamicVertex;
             i < g_liveTownCpuMesh.vertices.size(); ++i) {
            const auto& v = g_liveTownCpuMesh.vertices[i];
            g_vdp1Vertices[i] = v;
            g_vdp1LightingVertices[i] = v;
            g_vdp1TextureVertices[i].x = v.x;
            g_vdp1TextureVertices[i].y = v.y;
            g_vdp1TextureVertices[i].z = v.z;
            g_vdp1GouraudVertices[i].x = v.x;
            g_vdp1GouraudVertices[i].y = v.y;
            g_vdp1GouraudVertices[i].z = v.z;
        }
    }
    g_profileBuildUploadUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - tUpload);

    if (g_sceneGameMode == 3u) {
        static unsigned int fieldStreamHeartbeat = 0u;
        const unsigned int decodedTexturesAfter =
            static_cast<unsigned int>(
                g_staticRoomCpuMesh.decodedTextureData.size());
        const unsigned int buildElapsedUs =
            static_cast<unsigned int>(
                sceKernelGetProcessTimeWide() - buildStartUs);
        const bool periodicSample =
            (fieldStreamHeartbeat++ % 120u) == 0u;
        if (changed ||
            modelCacheMisses != 0u ||
            g_profileObjectMaterialCacheMisses != 0u ||
            decodedTexturesAfter != decodedTexturesBefore ||
            periodicSample) {
            logging::writef(
                "[FieldStream] frame=%llu submissions=%u polys=%u verts=%u "
                "modelMiss=%u matMiss=%u textures=%u->%u prepare=%u "
                "gpuTex=%u dirty=%u append=%uus material=%uus "
                "upload=%uus build=%uus reuseTex=%u reuseGeom=%u release=%uus "
                "baseAlloc=%uus wire=%uus texUpload=%uus texAlloc=%uus "
                "texBuild=%uus subAlloc=%uus subBuild=%uus copy=%uus\n",
                static_cast<unsigned long long>(
                    azel_bridge::published_frame_number()),
                g_liveTownSubmissionCount,
                static_cast<unsigned int>(
                    g_liveTownCpuMesh.polygonRecords.size()),
                static_cast<unsigned int>(
                    g_liveTownCpuMesh.vertices.size()),
                modelCacheMisses,
                g_profileObjectMaterialCacheMisses,
                decodedTexturesBefore,
                decodedTexturesAfter,
                changed ? 1u : 0u,
                static_cast<unsigned int>(g_vdp1GpuTextures.size()),
                g_vdp1TextureDataDirty ? 1u : 0u,
                g_profileObjectAppendUs,
                g_profileObjectMaterialResolveUs,
                g_profileBuildUploadUs,
                buildElapsedUs,
                g_profilePrepareReusedTextures ? 1u : 0u,
                g_profilePrepareReusedGeometry ? 1u : 0u,
                g_profilePrepareReleaseUs,
                g_profilePrepareBaseAllocUs,
                g_profilePrepareWireUs,
                g_profilePrepareTextureUploadUs,
                g_profilePrepareTexturedAllocUs,
                g_profilePrepareTexturedBuildUs,
                g_profilePrepareSubdivAllocUs,
                g_profilePrepareSubdivBuildUs,
                g_profilePrepareCopyUs);
        }
    }

    static bool loggedFirstLiveFrame = false;
    if (!loggedFirstLiveFrame) {
        logging::writef(
            "[SceneRender] live frame ready verts=%u polys=%u "
            "atlas=%u submissions=%u\n",
            static_cast<unsigned>(g_liveTownCpuMesh.vertices.size()),
            static_cast<unsigned>(g_liveTownCpuMesh.polygonRecords.size()),
            static_cast<unsigned>(
                g_staticRoomCpuMesh.decodedTextureData.size()),
            g_liveTownSubmissionCount);
        loggedFirstLiveFrame = true;
    }
    return true;
}

static ViewerMat4 buildAuthenticRoomWvp()
{
    const bool nativeCamera =
        g_townPlayerReady && g_townCameraReady;
    const ViewerMat4 view =
        viewerLookAtLH(
            nativeCamera
                ? g_townCameraPosition
                : g_staticRoomCpuMesh.cameraPosition,
            nativeCamera
                ? g_townCameraTarget
                : g_staticRoomCpuMesh.cameraTarget,
            nativeCamera
                ? g_townCameraUp
                : g_staticRoomCpuMesh.cameraUp);

    const bool nativeFieldClip =
        g_sceneGameMode == 3u &&
        g_nativeSceneNearPlane > 0.0f &&
        g_nativeSceneFarPlane > g_nativeSceneNearPlane;
    const float nearPlane =
        nativeFieldClip
            ? g_nativeSceneNearPlane
            : (g_staticRoomCpuReady && g_staticRoomCpuMesh.cameraNear > 0.0f
                ? g_staticRoomCpuMesh.cameraNear
                : static_cast<float>(0x999) / 65536.0f);
    const float farPlane =
        nativeFieldClip
            ? g_nativeSceneFarPlane
            : (g_staticRoomCpuReady && g_staticRoomCpuMesh.cameraFar > nearPlane
                ? g_staticRoomCpuMesh.cameraFar
                : static_cast<float>(0xF000) / 65536.0f);

    ViewerMat4 projection =
        buildAzelProjection(
            g_azelProjectionFovDegrees,
            0u,
            nearPlane,
            farPlane);

    // Town/room presentation needs the Saturn->GXM horizontal mirror.
    // Native field submissions already arrive in the correct horizontal
    // orientation through Azel's field camera-space path, so do not mirror
    // mode 3 a second time.
    if (g_sceneGameMode != 3u)
        projection.m[0] = -projection.m[0];

    return viewerMul(view, projection);
}

static ViewerMat4 buildViewerWvp(bool roomMode)
{
    constexpr float kDefaultFovDegrees = 80.0f;
    constexpr float kDefaultNear =
        static_cast<float>(0x999) / 65536.0f;
    constexpr float kDefaultFar =
        static_cast<float>(0x200000) / 65536.0f;

    const float* center =
        roomMode
            ? g_staticRoomViewCenter
            : g_basicWingViewCenter;

    const ViewerMat4 centerToOrigin =
        viewerTranslation(
            -center[0],
            -center[1],
            -center[2]);
    const ViewerMat4 yaw =
        viewerRotationY(g_viewYaw);
    const ViewerMat4 pitch =
        viewerRotationX(g_viewPitch);
    const ViewerMat4 camera =
        viewerTranslation(
            0.0f,
            0.0f,
            g_viewDistance);
    ViewerMat4 projection =
        buildAzelProjection(
            kDefaultFovDegrees,
            0u,
            kDefaultNear,
            kDefaultFar);

    // All Saturn/Azel-authored 3D uses the same presentation conversion.
    // This includes diagnostic model views as well as room/scene views.
    projection.m[0] = -projection.m[0];

    // Geometry remains in native Azel game space. Debug framing is purely a
    // view transform: translate the model/scene center to the origin, orbit
    // it, then place the camera far enough away to fit it.
    return viewerMul(
        viewerMul(
            viewerMul(
                viewerMul(
                    centerToOrigin,
                    yaw),
                pitch),
            camera),
        projection);
}

static void computeDebugFrame(
    const std::vector<azel::DebugColorVertex>& vertices,
    float outCenter[3],
    float& outDistance)
{
    if (vertices.empty()) {
        outCenter[0] = outCenter[1] = outCenter[2] = 0.0f;
        outDistance = 3.0f;
        return;
    }

    float minV[3] = {
        vertices[0].x,
        vertices[0].y,
        vertices[0].z
    };
    float maxV[3] = {
        vertices[0].x,
        vertices[0].y,
        vertices[0].z
    };

    for (const auto& v : vertices) {
        minV[0] = std::min(minV[0], v.x);
        minV[1] = std::min(minV[1], v.y);
        minV[2] = std::min(minV[2], v.z);
        maxV[0] = std::max(maxV[0], v.x);
        maxV[1] = std::max(maxV[1], v.y);
        maxV[2] = std::max(maxV[2], v.z);
    }

    for (unsigned int i = 0; i < 3; ++i)
        outCenter[i] = (minV[i] + maxV[i]) * 0.5f;

    float radiusSq = 0.0f;
    for (const auto& v : vertices) {
        const float dx = v.x - outCenter[0];
        const float dy = v.y - outCenter[1];
        const float dz = v.z - outCenter[2];
        radiusSq = std::max(
            radiusSq,
            dx*dx + dy*dy + dz*dz);
    }

    const float radius =
        std::max(std::sqrt(radiusSq), 0.01f);

    float xScale = 1.0f;
    float yScale = 1.0f;
    azelProjectionScales(
        80.0f,
        0u,
        xScale,
        yScale);

    // Keep a little margin and account for the bounding sphere extending
    // toward the camera. This changes only camera distance, never model scale.
    outDistance =
        radius +
        radius * std::max(xScale, yScale) / 0.72f;
}

struct ViewerScreenPoint
{
    float x = 0.0f;
    float y = 0.0f;
    bool valid = false;
};

struct GouraudQuadPrep
{
    bool visible = false;
    ViewerScreenPoint screen[4]{};
};

static std::vector<GouraudQuadPrep> g_liveTownGouraudPrep;
static const azel::DebugColorVertex* g_liveTownGouraudPrepVertices = nullptr;
static std::size_t g_liveTownGouraudPrepPolygonCount = 0;
static bool g_liveTownGouraudPrepValid = false;

static int viewerRenderWidth()
{
    return g_halfResolution ? (kWidth / 2) : kWidth;
}

static int viewerRenderHeight()
{
    return g_halfResolution ? (kHeight / 2) : kHeight;
}

static int viewerRenderPitch()
{
    return g_halfResolution ? 512 : 1024;
}

static ViewerScreenPoint projectViewerPoint(
    const ViewerMat4& wvp,
    const azel::DebugColorVertex& v)
{
    // Shader uses mul(float4(position,1), wvp): row-vector convention.
    const float clipX =
        v.x * wvp.m[0] + v.y * wvp.m[4] +
        v.z * wvp.m[8] + wvp.m[12];
    const float clipY =
        v.x * wvp.m[1] + v.y * wvp.m[5] +
        v.z * wvp.m[9] + wvp.m[13];
    const float clipW =
        v.x * wvp.m[3] + v.y * wvp.m[7] +
        v.z * wvp.m[11] + wvp.m[15];

    if (clipW <= 0.00001f)
        return {};

    const float ndcX = clipX / clipW;
    const float ndcY = clipY / clipW;

    ViewerScreenPoint out{};
    out.x = (ndcX * 0.5f + 0.5f) *
        static_cast<float>(viewerRenderWidth());
    out.y = (0.5f - ndcY * 0.5f) *
        static_cast<float>(viewerRenderHeight());
    out.valid = true;
    return out;
}

// Conservative homogeneous clip test for the Vita backend. This does not
// replace Azel's town visibility/LOD pipeline: Azel has already selected the
// cells, objects and models that reach this point. It only avoids referencing
// a submitted quad when all four of its corners are outside the same hardware
// clip plane, which is equivalent to work GXM would otherwise discard later.
static std::uint8_t viewerClipOutcode(
    const ViewerMat4& wvp,
    const azel::DebugColorVertex& v)
{
    const float clipX =
        v.x * wvp.m[0] + v.y * wvp.m[4] +
        v.z * wvp.m[8] + wvp.m[12];
    const float clipY =
        v.x * wvp.m[1] + v.y * wvp.m[5] +
        v.z * wvp.m[9] + wvp.m[13];
    const float clipZ =
        v.x * wvp.m[2] + v.y * wvp.m[6] +
        v.z * wvp.m[10] + wvp.m[14];
    const float clipW =
        v.x * wvp.m[3] + v.y * wvp.m[7] +
        v.z * wvp.m[11] + wvp.m[15];

    std::uint8_t out = 0;
    if (clipX < -clipW) out |= 1u << 0; // left
    if (clipX >  clipW) out |= 1u << 1; // right
    if (clipY < -clipW) out |= 1u << 2; // bottom
    if (clipY >  clipW) out |= 1u << 3; // top
    if (clipZ <  0.0f)  out |= 1u << 4; // near (GXM/D3D-style 0..W Z)
    if (clipZ >  clipW) out |= 1u << 5; // far
    return out;
}

static void prepareLiveTownGouraudVisibility(const ViewerMat4& wvp)
{
    const std::uint64_t prepStartUs = sceKernelGetProcessTimeWide();
    static const unsigned int cornerVertex[4] = {0u, 1u, 2u, 5u};
    const std::size_t polygonCount =
        g_liveTownCpuMesh.polygonRecords.size();

    g_liveTownGouraudPrep.assign(polygonCount, GouraudQuadPrep{});
    g_liveTownGouraudPrepVertices = g_liveTownCpuMesh.vertices.data();
    g_liveTownGouraudPrepPolygonCount = polygonCount;
    g_liveTownGouraudPrepValid =
        polygonCount != 0u &&
        g_liveTownCpuMesh.vertices.size() == polygonCount * 6u;
    g_profileGouraudVisibleQuads = 0u;
    g_profileGouraudTotalQuads = static_cast<unsigned int>(polygonCount);

    if (!g_liveTownGouraudPrepValid) {
        g_profileGouraudPrepUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - prepStartUs);
        return;
    }

    for (std::size_t p = 0; p < polygonCount; ++p) {
        auto& prep = g_liveTownGouraudPrep[p];
        bool projectable = true;
        std::uint8_t sharedOutcode = 0x3Fu;

        for (unsigned int corner = 0; corner < 4u; ++corner) {
            const auto& v = g_liveTownCpuMesh.vertices[
                p * 6u + cornerVertex[corner]];
            const float clipX =
                v.x * wvp.m[0] + v.y * wvp.m[4] +
                v.z * wvp.m[8] + wvp.m[12];
            const float clipY =
                v.x * wvp.m[1] + v.y * wvp.m[5] +
                v.z * wvp.m[9] + wvp.m[13];
            const float clipZ =
                v.x * wvp.m[2] + v.y * wvp.m[6] +
                v.z * wvp.m[10] + wvp.m[14];
            const float clipW =
                v.x * wvp.m[3] + v.y * wvp.m[7] +
                v.z * wvp.m[11] + wvp.m[15];

            std::uint8_t outcode = 0u;
            if (clipX < -clipW) outcode |= 1u << 0;
            if (clipX >  clipW) outcode |= 1u << 1;
            if (clipY < -clipW) outcode |= 1u << 2;
            if (clipY >  clipW) outcode |= 1u << 3;
            if (clipZ <  0.0f)  outcode |= 1u << 4;
            if (clipZ >  clipW) outcode |= 1u << 5;
            sharedOutcode &= outcode;

            if (clipW <= 0.00001f) {
                projectable = false;
                continue;
            }

            const float invW = 1.0f / clipW;
            prep.screen[corner].x =
                (clipX * invW * 0.5f + 0.5f) *
                static_cast<float>(viewerRenderWidth());
            prep.screen[corner].y =
                (0.5f - clipY * invW * 0.5f) *
                static_cast<float>(viewerRenderHeight());
            prep.screen[corner].valid = true;
        }

        prep.visible = projectable && sharedOutcode == 0u;
        if (prep.visible)
            ++g_profileGouraudVisibleQuads;
    }

    g_profileGouraudPrepUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - prepStartUs);
}

static void generateAzelFalloff(
    std::uint32_t r4,
    std::uint32_t r5,
    std::uint32_t r6,
    std::int16_t out[32][3])
{
    auto s8 = [](std::uint32_t value) -> std::int32_t {
        return static_cast<std::int8_t>(value & 0xFFu);
    };

    std::int32_t r9t =
        0x8421 * (s8(r5) - s8(r6));
    std::int32_t r10t =
        0x8421 * (s8(r5 >> 8) - s8(r6 >> 8));
    std::int32_t r11t =
        0x8421 * (s8(r5 >> 16) - s8(r6 >> 16));

    std::int32_t r1t =
        0x84210 * (s8(r6) - s8(r4));
    std::int32_t r5t = s8(r4) << 24;
    std::int32_t r2t =
        0x84210 * (s8(r6 >> 8) - s8(r4 >> 8));
    std::int32_t r6t = s8(r4 >> 8) << 24;
    std::int32_t r3t =
        0x84210 * (s8(r6 >> 16) - s8(r4 >> 16));
    std::int32_t r7t = s8(r4 >> 16) << 24;

    for (int i = 0; i < 32; ++i) {
        out[i][0] =
            static_cast<std::int16_t>(
                static_cast<std::uint32_t>(r5t) >> 16);
        out[i][1] =
            static_cast<std::int16_t>(
                static_cast<std::uint32_t>(r6t) >> 16);
        out[i][2] =
            static_cast<std::int16_t>(
                static_cast<std::uint32_t>(r7t) >> 16);

        r1t += r9t;
        r2t += r10t;
        r3t += r11t;

        r5t += r1t;
        r6t += r2t;
        r7t += r3t;
    }
}

static void generateViewerAzelFalloff(
    std::int16_t out[32][3])
{
    generateAzelFalloff(
        0x00030102u,
        0x00000000u,
        0x00000000u,
        out);
}

static void updateViewerAzelLighting()
{
    if (g_basicWingCpuMesh.lightingNormals.size() !=
            g_basicWingCpuMesh.polygons ||
        g_basicWingCpuMesh.gouraud555.size() !=
            g_basicWingCpuMesh.polygons)
        return;

    // Mirror the Basic Wing menu's actual Azel light setup:
    //     setupLight(0, 0, 0x10000, 0x161918)
    // setupLight stores the directional vector as -input >> 4, so the
    // camera-space light used by ComputeColorFromNormal is (0,0,-4096).
    // The RGB channel ordering below matches Azel's reversed storage.
    constexpr int lightR = 0x18;
    constexpr int lightG = 0x19;
    constexpr int lightB = 0x16;

    static bool falloffReady = false;
    static std::int16_t falloffMap[32][3]{};
    if (!falloffReady) {
        generateViewerAzelFalloff(falloffMap);
        falloffReady = true;
    }

    const ViewerMat4 rotation =
        viewerMul(
            viewerRotationY(g_viewYaw),
            viewerRotationX(g_viewPitch));

    auto transformDirection = [&](float x, float y, float z) {
        struct V { float x, y, z; } out;
        out.x = x * rotation.m[0] +
                y * rotation.m[4] +
                z * rotation.m[8];
        out.y = x * rotation.m[1] +
                y * rotation.m[5] +
                z * rotation.m[9];
        out.z = x * rotation.m[2] +
                y * rotation.m[6] +
                z * rotation.m[10];
        const float lenSq =
            out.x * out.x + out.y * out.y + out.z * out.z;
        if (lenSq > 0.0000001f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            out.x *= invLen;
            out.y *= invLen;
            out.z *= invLen;
        }
        return out;
    };

    auto viewDepthForQuad = [&](unsigned int p) {
        const auto& v = g_basicWingCpuMesh.vertices[p * 6u];

        // The debug camera now frames native Azel game-space geometry by
        // translating the model bounds center in the view matrix rather than
        // rescaling the model itself.
        const float x =
            v.x - g_basicWingViewCenter[0];
        const float y =
            v.y - g_basicWingViewCenter[1];
        const float z0 =
            v.z - g_basicWingViewCenter[2];

        const float z =
            x * rotation.m[2] +
            y * rotation.m[6] +
            z0 * rotation.m[10] +
            g_viewDistance;
        return std::fabs(z);
    };

    for (unsigned int p = 0;
         p < g_basicWingCpuMesh.polygons; ++p) {
        // The dragon morph viewer uses a 16.0 far clip. Azel's
        // GetDistanceFalloff mapping with that setup is effectively two
        // falloff-table entries per view-space unit:
        // index = clamp(floor(depth * 2), 0, 31).
        const int falloffIndex =
            std::max(
                0,
                std::min(
                    31,
                    static_cast<int>(
                        viewDepthForQuad(p) * 2.0f)));

        const int fallR = falloffMap[falloffIndex][0];
        const int fallG = falloffMap[falloffIndex][1];
        const int fallB = falloffMap[falloffIndex][2];

        for (unsigned int corner = 0; corner < 4; ++corner) {
            const auto& source =
                g_basicWingCpuMesh.lightingNormals[p].corner[corner];

            // Keep the light fixed relative to the camera by rotating the
            // posed model-space normal into view space every frame.
            const auto n =
                transformDirection(source[0], source[1], source[2]);

            // Azel's camera-space light vector is (0,0,-4096). With a
            // normalized normal represented at Saturn scale 4096, the high
            // word of the fixed-point dot is approximately 256 * max(-Nz,0).
            const float dot = std::max(0.0f, -n.z);
            const int dotHi =
                static_cast<int>(dot * 256.0f);

            int accum[3] = {fallR, fallG, fallB};
            if (dotHi > 0) {
                accum[0] += lightR * dotHi;
                accum[1] += lightG * dotHi;
                accum[2] += lightB * dotHi;
            }

            for (int channel = 0; channel < 3; ++channel) {
                accum[channel] =
                    std::max(0, std::min(0x1F00, accum[channel]));
                const int gouraud5 =
                    (accum[channel] >> 8) & 0x1F;
                g_basicWingCpuMesh.gouraud555[p]
                    .corner[corner][channel] =
                    (static_cast<float>(gouraud5) - 16.0f) /
                    31.0f;
            }
        }
    }
}

static void updateStaticRoomAzelLighting(bool authenticDepth)
{
    if (!g_staticRoomCpuReady ||
        !g_staticRoomCpuMesh.lightingValid ||
        g_staticRoomCpuMesh.gouraud555.size() !=
            g_staticRoomCpuMesh.polygons)
        return;

    std::int16_t falloffMap[32][3]{};
    generateAzelFalloff(
        g_staticRoomCpuMesh.lightFalloff[0],
        g_staticRoomCpuMesh.lightFalloff[1],
        g_staticRoomCpuMesh.lightFalloff[2],
        falloffMap);

    float cameraForward[3] = {
        g_staticRoomCpuMesh.cameraTarget[0] -
            g_staticRoomCpuMesh.cameraPosition[0],
        g_staticRoomCpuMesh.cameraTarget[1] -
            g_staticRoomCpuMesh.cameraPosition[1],
        g_staticRoomCpuMesh.cameraTarget[2] -
            g_staticRoomCpuMesh.cameraPosition[2]
    };
    const float forwardLenSq =
        cameraForward[0]*cameraForward[0] +
        cameraForward[1]*cameraForward[1] +
        cameraForward[2]*cameraForward[2];
    if (forwardLenSq > 0.0000001f) {
        const float inv = 1.0f / std::sqrt(forwardLenSq);
        cameraForward[0] *= inv;
        cameraForward[1] *= inv;
        cameraForward[2] *= inv;
    }

    auto falloffIndexForQuad = [&](unsigned int p) {
        if (!authenticDepth ||
            !g_staticRoomCpuMesh.cameraValid ||
            g_staticRoomCpuMesh.worldVertices.size() <
                (p * 6u + 1u))
            return 0;

        const auto& v =
            g_staticRoomCpuMesh.worldVertices[p * 6u];

        const float dx =
            v.x - g_staticRoomCpuMesh.cameraPosition[0];
        const float dy =
            v.y - g_staticRoomCpuMesh.cameraPosition[1];
        const float dz =
            v.z - g_staticRoomCpuMesh.cameraPosition[2];

        const float depth =
            std::fabs(
                dx * cameraForward[0] +
                dy * cameraForward[1] +
                dz * cameraForward[2]);

        const std::int64_t rawDepth =
            static_cast<std::int64_t>(
                std::llround(depth * 65536.0f));

        // Match Azel drawObject() + GetDistanceFalloff():
        // computeViewDepth returns abs(viewZ) << 8, then GetDistanceFalloff
        // multiplies by oneOverFarClip256 and indexes the 32-entry table.
        const std::int64_t viewDepth =
            rawDepth << 8;
        const std::int64_t farRaw =
            static_cast<std::int64_t>(
                std::llround(
                    g_staticRoomCpuMesh.cameraFar * 65536.0f));
        if (farRaw <= 0)
            return 0;

        const std::int64_t oneOverFar =
            (static_cast<std::int64_t>(0x8000) << 16) /
            farRaw;
        const std::int64_t oneOverFar256 =
            oneOverFar << 8;
        std::int64_t scaledDepth =
            (viewDepth * oneOverFar256) >> 32;
        if (scaledDepth < 0)
            scaledDepth = 0;

        const int byteOffset =
            (static_cast<int>((scaledDepth << 1) >> 8)) & ~7;
        return std::max(0, std::min(31, byteOffset >> 3));
    };

    const int lightVector[3] = {
        static_cast<int>(std::lround(
            -g_staticRoomCpuMesh.lightDirection[0] * 4096.0f)),
        static_cast<int>(std::lround(
            -g_staticRoomCpuMesh.lightDirection[1] * 4096.0f)),
        static_cast<int>(std::lround(
            -g_staticRoomCpuMesh.lightDirection[2] * 4096.0f))
    };

    for (unsigned int p = 0;
         p < g_staticRoomCpuMesh.polygons; ++p) {
        const auto& record =
            g_staticRoomCpuMesh.polygonRecords[p];
        const unsigned int mode =
            (record.lightingControl >> 8) & 3u;

        auto& out = g_staticRoomCpuMesh.gouraud555[p];
        out = {};

        if (mode == 0u || record.lightingCount == 0u)
            continue;

        for (unsigned int corner = 0; corner < 4u; ++corner) {
            const unsigned int normalIndex =
                mode == 1u ? 0u : corner;
            if (normalIndex >= record.lightingCount)
                continue;

            const auto& lighting =
                record.lighting[normalIndex];

            int dotProduct =
                static_cast<int>(lighting.normal[0]) * lightVector[0] +
                static_cast<int>(lighting.normal[1]) * lightVector[1] +
                static_cast<int>(lighting.normal[2]) * lightVector[2];

            const int falloffIndex =
                falloffIndexForQuad(p);
            int accum[3] = {
                falloffMap[falloffIndex][0],
                falloffMap[falloffIndex][1],
                falloffMap[falloffIndex][2]
            };

            if (mode == 2u && lighting.hasColor) {
                accum[0] +=
                    static_cast<std::int16_t>(lighting.color[0]);
                accum[1] +=
                    static_cast<std::int16_t>(lighting.color[1]);
                accum[2] +=
                    static_cast<std::int16_t>(lighting.color[2]);
            }

            if (dotProduct > 0) {
                const int dotHi =
                    static_cast<int>(
                        static_cast<std::uint32_t>(
                            dotProduct) >> 16);
                accum[0] +=
                    static_cast<int>(
                        g_staticRoomCpuMesh.lightColor[0]) * dotHi;
                accum[1] +=
                    static_cast<int>(
                        g_staticRoomCpuMesh.lightColor[1]) * dotHi;
                accum[2] +=
                    static_cast<int>(
                        g_staticRoomCpuMesh.lightColor[2]) * dotHi;
            }

            for (int channel = 0; channel < 3; ++channel) {
                accum[channel] =
                    std::max(
                        0,
                        std::min(
                            0x1F00,
                            accum[channel]));
                const int gouraud5 =
                    (accum[channel] >> 8) & 0x1F;
                out.corner[corner][channel] =
                    (static_cast<float>(gouraud5) - 16.0f) /
                    31.0f;
            }
        }
    }
}

void toggle_debug_console()
{
    // Plain Select toggles only the lightweight performance HUD.
    if (!g_viewerReady)
        return;

    if (g_renderThreadStarted && g_renderFrameFreeSema >= 0)
        sceKernelWaitSema(g_renderFrameFreeSema, 1, nullptr);
    g_showThreadTimingOsd = !g_showThreadTimingOsd;
    if (g_renderThreadStarted && g_renderFrameFreeSema >= 0)
        sceKernelSignalSema(g_renderFrameFreeSema, 1);
}

void toggle_full_debug_screen()
{
    if (!g_viewerReady || !g_gxmProbeAttempted)
        return;

    if (g_renderThreadStarted && g_renderFrameFreeSema >= 0)
        sceKernelWaitSema(g_renderFrameFreeSema, 1, nullptr);

    g_debugVisible = !g_debugVisible;
    g_probeDisplayingGxm = !g_debugVisible;

    if (g_renderThreadStarted && g_renderFrameFreeSema >= 0)
        sceKernelSignalSema(g_renderFrameFreeSema, 1);
}

void show_game_presentation()
{
    // One-way handoff from the loading framebuffer to native GXM town
    // presentation. The old diagnostic screen is intentionally not toggled
    // back in during play.
    if (g_gxmProbeAttempted) {
        if (!g_viewerReady)
            return;
        g_debugVisible = false;
        g_probeDisplayingGxm = true;
        return;
    }

    g_gxmProbeAttempted = true;
    g_suppressGxmInitPassStatus = true;

    SceGxmInitializeParams params{};
    params.flags = 0;
    params.displayQueueMaxPendingCount = 1;
    params.displayQueueCallback = nullptr;
    params.displayQueueCallbackDataSize = 0;
    params.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

    const int initResult = sceGxmInitialize(&params);
    if (initResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM INIT 0X%08X",
                      static_cast<unsigned int>(initResult));
        failure(line);
        return;
    }

    g_gxmInitialized = true;
    status("[PASS] GXM INITIALIZE", 0xFF80E0FFu);

    g_probeVdm = probeGpuAlloc(
        SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
        SCE_GXM_MEMORY_ATTRIB_READ, &g_probeVdmUid);
    g_probeVertex = probeGpuAlloc(
        SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
        SCE_GXM_MEMORY_ATTRIB_READ, &g_probeVertexUid);
    g_probeFragment = probeGpuAlloc(
        SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
        SCE_GXM_MEMORY_ATTRIB_READ, &g_probeFragmentUid);

    unsigned int fragmentUsseOffset = 0;
    g_probeFragmentUsse = probeFragmentUsseAlloc(
        SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
        &g_probeFragmentUsseUid, &fragmentUsseOffset);

    if (!g_probeVdm || !g_probeVertex || !g_probeFragment ||
        !g_probeFragmentUsse) {
        failure("[FAIL] GXM RING MEMORY");
        return;
    }

    status("[PASS] GXM RING MEMORY", 0xFF80E0FFu);

    g_probeContextHost = std::malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
    if (!g_probeContextHost) {
        failure("[FAIL] GXM CONTEXT HOST MEM");
        return;
    }

    SceGxmContextParams contextParams{};
    contextParams.hostMem = g_probeContextHost;
    contextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
    contextParams.vdmRingBufferMem = g_probeVdm;
    contextParams.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
    contextParams.vertexRingBufferMem = g_probeVertex;
    contextParams.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
    contextParams.fragmentRingBufferMem = g_probeFragment;
    contextParams.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
    contextParams.fragmentUsseRingBufferMem = g_probeFragmentUsse;
    contextParams.fragmentUsseRingBufferMemSize =
        SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
    contextParams.fragmentUsseRingBufferOffset = fragmentUsseOffset;

    const int contextResult =
        sceGxmCreateContext(&contextParams, &g_probeContext);
    if (contextResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM CONTEXT 0X%08X",
                      static_cast<unsigned int>(contextResult));
        failure(line);
        return;
    }

    status("[PASS] GXM CONTEXT", 0xFF80E0FFu);

    SceGxmRenderTargetParams rtParams{};
    rtParams.flags = 0;
    rtParams.width = kWidth;
    rtParams.height = kHeight;
    rtParams.scenesPerFrame = 1;
    rtParams.multisampleMode = kMultisampleMode;
    rtParams.multisampleLocations = 0;
    rtParams.driverMemBlock = -1;

    const int rtResult =
        sceGxmCreateRenderTarget(&rtParams, &g_probeRenderTarget);
    if (rtResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM TARGET 0X%08X",
                      static_cast<unsigned int>(rtResult));
        failure(line);
        std::printf("[GXM] sceGxmCreateRenderTarget failed: 0x%08X\n",
                    static_cast<unsigned int>(rtResult));
        return;
    }

    status("[PASS] GXM RENDER TARGET", 0xFF80E0FFu);

    SceGxmRenderTargetParams halfRtParams = rtParams;
    halfRtParams.width = kWidth / 2;
    halfRtParams.height = kHeight / 2;

    const int halfRtResult =
        sceGxmCreateRenderTarget(&halfRtParams, &g_probeRenderTargetHalf);
    if (halfRtResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM HALF TARGET 0X%08X",
                      static_cast<unsigned int>(halfRtResult));
        failure(line);
        return;
    }

    status("[PASS] GXM HALF RENDER TARGET", 0xFF80E0FFu);

    SceGxmRenderTargetParams movieRtParams = halfRtParams;
    movieRtParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
    const int movieRtResult =
        sceGxmCreateRenderTarget(&movieRtParams, &g_movieRenderTarget);
    if (movieRtResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM MOVIE TARGET 0X%08X",
                      static_cast<unsigned int>(movieRtResult));
        failure(line);
        return;
    }
    status("[PASS] GXM MOVIE TARGET NO-MSAA", 0xFF80E0FFu);

    SceGxmRenderTargetParams frontendHighRtParams = movieRtParams;
    frontendHighRtParams.width = 720;
    frontendHighRtParams.height = 408;
    const int frontendHighRtResult =
        sceGxmCreateRenderTarget(
            &frontendHighRtParams, &g_frontendHighRenderTarget);
    if (frontendHighRtResult < 0) {
        char line[78];
        std::snprintf(
            line, sizeof(line), "[FAIL] GXM FRONTEND 720X408 TARGET 0X%08X",
            static_cast<unsigned int>(frontendHighRtResult));
        failure(line);
        return;
    }
    status("[PASS] GXM FRONTEND 720X408 TARGET", 0xFF80E0FFu);

    constexpr int gxmPitch = 1024;
    constexpr unsigned int colorBytes =
        static_cast<unsigned int>(gxmPitch * kHeight * sizeof(std::uint32_t));

    g_probeColorBuffer = static_cast<std::uint32_t*>(
        probeCdramAlloc(
            colorBytes,
            SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
            &g_probeColorUid));
    if (!g_probeColorBuffer) {
        failure("[FAIL] GXM COLOR MEMORY");
        return;
    }

    std::memset(g_probeColorBuffer, 0, colorBytes);

    const int colorResult = sceGxmColorSurfaceInit(
        &g_probeColorSurface,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        kWidth, kHeight, gxmPitch, g_probeColorBuffer);
    if (colorResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM COLOR 0X%08X",
                      static_cast<unsigned int>(colorResult));
        failure(line);
        return;
    }

    const int halfColorResult = sceGxmColorSurfaceInit(
        &g_probeColorSurfaceHalf,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        kWidth / 2, kHeight / 2, 512, g_probeColorBuffer);
    if (halfColorResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM HALF COLOR 0X%08X",
                      static_cast<unsigned int>(halfColorResult));
        failure(line);
        return;
    }

    const int movieColorResult = sceGxmColorSurfaceInit(
        &g_movieColorSurface,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        kWidth / 2, kHeight / 2, 512, g_probeColorBuffer);
    if (movieColorResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM MOVIE COLOR 0X%08X",
                      static_cast<unsigned int>(movieColorResult));
        failure(line);
        return;
    }

    const int frontendHighColorResult = sceGxmColorSurfaceInit(
        &g_frontendHighColorSurface,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        720, 408, gxmPitch, g_probeColorBuffer);
    if (frontendHighColorResult < 0) {
        char line[78];
        std::snprintf(
            line, sizeof(line), "[FAIL] GXM FRONTEND 720X408 COLOR 0X%08X",
            static_cast<unsigned int>(frontendHighColorResult));
        failure(line);
        return;
    }

    if (sceGxmSyncObjectCreate(&g_probeSync) < 0) {
        failure("[FAIL] GXM SYNC OBJECT");
        return;
    }

    // Interactive viewer uses a second GXM scanout surface so the GPU never
    // redraws the buffer currently being scanned out.
    g_probeColorBuffer2 = static_cast<std::uint32_t*>(
        probeCdramAlloc(
            colorBytes,
            SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
            &g_probeColorUid2));
    if (!g_probeColorBuffer2) {
        failure("[FAIL] GXM COLOR MEMORY 2");
        return;
    }

    std::memset(g_probeColorBuffer2, 0, colorBytes);

    const int colorResult2 = sceGxmColorSurfaceInit(
        &g_probeColorSurface2,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        kWidth, kHeight, gxmPitch, g_probeColorBuffer2);
    if (colorResult2 < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM COLOR2 0X%08X",
                      static_cast<unsigned int>(colorResult2));
        failure(line);
        return;
    }

    const int halfColorResult2 = sceGxmColorSurfaceInit(
        &g_probeColorSurfaceHalf2,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        kWidth / 2, kHeight / 2, 512, g_probeColorBuffer2);
    if (halfColorResult2 < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM HALF COLOR2 0X%08X",
                      static_cast<unsigned int>(halfColorResult2));
        failure(line);
        return;
    }

    const int movieColorResult2 = sceGxmColorSurfaceInit(
        &g_movieColorSurface2,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        kWidth / 2, kHeight / 2, 512, g_probeColorBuffer2);
    if (movieColorResult2 < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM MOVIE COLOR2 0X%08X",
                      static_cast<unsigned int>(movieColorResult2));
        failure(line);
        return;
    }

    const int frontendHighColorResult2 = sceGxmColorSurfaceInit(
        &g_frontendHighColorSurface2,
        SCE_GXM_COLOR_FORMAT_A8B8G8R8,
        SCE_GXM_COLOR_SURFACE_LINEAR,
        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
        720, 408, gxmPitch, g_probeColorBuffer2);
    if (frontendHighColorResult2 < 0) {
        char line[78];
        std::snprintf(
            line, sizeof(line), "[FAIL] GXM FRONTEND 720X408 COLOR2 0X%08X",
            static_cast<unsigned int>(frontendHighColorResult2));
        failure(line);
        return;
    }

    if (sceGxmSyncObjectCreate(&g_probeSync2) < 0) {
        failure("[FAIL] GXM SYNC OBJECT 2");
        return;
    }

    status("[PASS] GXM COLOR SURFACES X2", 0xFF80E0FFu);

    constexpr unsigned int cinepakResolvePitch = 512u;
    constexpr unsigned int cinepakResolveWidth = kWidth / 2;
    constexpr unsigned int cinepakResolveHeight = kHeight / 2;
    constexpr unsigned int cinepakResolveBytes =
        cinepakResolvePitch * cinepakResolveHeight * sizeof(std::uint32_t);
    g_cinepakResolveColorBuffer = static_cast<std::uint32_t*>(
        probeCdramAlloc(
            cinepakResolveBytes,
            SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
            &g_cinepakResolveColorUid));
    if (!g_cinepakResolveColorBuffer) {
        failure("[FAIL] CINEPAK RESOLVE MEMORY");
        return;
    }
    std::memset(g_cinepakResolveColorBuffer, 0, cinepakResolveBytes);

    if (sceGxmColorSurfaceInit(
            &g_cinepakResolveColorSurface,
            SCE_GXM_COLOR_FORMAT_A8B8G8R8,
            SCE_GXM_COLOR_SURFACE_LINEAR,
            SCE_GXM_COLOR_SURFACE_SCALE_NONE,
            SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
            cinepakResolveWidth,
            cinepakResolveHeight,
            cinepakResolvePitch,
            g_cinepakResolveColorBuffer) < 0) {
        failure("[FAIL] CINEPAK RESOLVE SURFACE");
        return;
    }
    status("[PASS] CINEPAK RESOLVE SURFACE", 0xFF80E0FFu);

    const unsigned int alignedW =
        (kWidth + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1);
    const unsigned int alignedH =
        (kHeight + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1);
    // Single-sample depth/stencil storage matches the globally disabled MSAA
    // configuration.
    const unsigned int samples = alignedW * alignedH;

    g_probeDepth = probeGpuAlloc(
        4u * samples,
        SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
        &g_probeDepthUid);
    g_probeStencil = probeGpuAlloc(
        4u * samples,
        SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
        &g_probeStencilUid);
    if (!g_probeDepth || !g_probeStencil) {
        failure("[FAIL] GXM DEPTH MEMORY");
        return;
    }

    const int depthResult = sceGxmDepthStencilSurfaceInit(
        &g_probeDepthSurface,
        SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
        SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
        alignedW,
        g_probeDepth,
        g_probeStencil);
    if (depthResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM DEPTH 0X%08X",
                      static_cast<unsigned int>(depthResult));
        failure(line);
        return;
    }

    const unsigned int halfAlignedW =
        ((kWidth / 2) + SCE_GXM_TILE_SIZEX - 1) &
        ~(SCE_GXM_TILE_SIZEX - 1);
    const int halfDepthResult = sceGxmDepthStencilSurfaceInit(
        &g_probeDepthSurfaceHalf,
        SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
        SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
        halfAlignedW,
        g_probeDepth,
        g_probeStencil);
    if (halfDepthResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM HALF DEPTH 0X%08X",
                      static_cast<unsigned int>(halfDepthResult));
        failure(line);
        return;
    }

    status("[PASS] GXM DEPTH SURFACES", 0xFF80E0FFu);

    constexpr unsigned int patcherBufferSize = 64u * 1024u;
    constexpr unsigned int patcherVertexUsseSize = 64u * 1024u;
    constexpr unsigned int patcherFragmentUsseSize = 64u * 1024u;

    g_probePatcherBuffer = probeGpuAlloc(
        patcherBufferSize,
        SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
        &g_probePatcherBufferUid);

    unsigned int patcherVertexUsseOffset = 0;
    g_probePatcherVertexUsse = probeVertexUsseAlloc(
        patcherVertexUsseSize,
        &g_probePatcherVertexUsseUid,
        &patcherVertexUsseOffset);

    unsigned int patcherFragmentUsseOffset = 0;
    g_probePatcherFragmentUsse = probeFragmentUsseAlloc(
        patcherFragmentUsseSize,
        &g_probePatcherFragmentUsseUid,
        &patcherFragmentUsseOffset);

    if (!g_probePatcherBuffer ||
        !g_probePatcherVertexUsse ||
        !g_probePatcherFragmentUsse) {
        failure("[FAIL] GXM PATCHER MEMORY");
        return;
    }

    status("[PASS] GXM PATCHER MEMORY", 0xFF80E0FFu);

    SceGxmShaderPatcherParams patcherParams{};
    patcherParams.userData = nullptr;
    patcherParams.hostAllocCallback = probePatcherHostAlloc;
    patcherParams.hostFreeCallback = probePatcherHostFree;
    patcherParams.bufferAllocCallback = nullptr;
    patcherParams.bufferFreeCallback = nullptr;
    patcherParams.bufferMem = g_probePatcherBuffer;
    patcherParams.bufferMemSize = patcherBufferSize;
    patcherParams.vertexUsseAllocCallback = nullptr;
    patcherParams.vertexUsseFreeCallback = nullptr;
    patcherParams.vertexUsseMem = g_probePatcherVertexUsse;
    patcherParams.vertexUsseMemSize = patcherVertexUsseSize;
    patcherParams.vertexUsseOffset = patcherVertexUsseOffset;
    patcherParams.fragmentUsseAllocCallback = nullptr;
    patcherParams.fragmentUsseFreeCallback = nullptr;
    patcherParams.fragmentUsseMem = g_probePatcherFragmentUsse;
    patcherParams.fragmentUsseMemSize = patcherFragmentUsseSize;
    patcherParams.fragmentUsseOffset = patcherFragmentUsseOffset;

    const int patcherResult =
        sceGxmShaderPatcherCreate(&patcherParams, &g_probeShaderPatcher);
    if (patcherResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM PATCHER 0X%08X",
                      static_cast<unsigned int>(patcherResult));
        failure(line);
        std::printf("[GXM] sceGxmShaderPatcherCreate failed: 0x%08X\n",
                    static_cast<unsigned int>(patcherResult));
        return;
    }

    status("[PASS] GXM SHADER PATCHER", 0xFF80E0FFu);

    const SceGxmProgram* vertexProgram =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_color_v_gxp_start);
    const SceGxmProgram* fragmentProgram =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_color_f_gxp_start);

    const int vertexCheck = sceGxmProgramCheck(vertexProgram);
    if (vertexCheck < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM VP CHECK 0X%08X",
                      static_cast<unsigned int>(vertexCheck));
        failure(line);
        return;
    }

    const int fragmentCheck = sceGxmProgramCheck(fragmentProgram);
    if (fragmentCheck < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM FP CHECK 0X%08X",
                      static_cast<unsigned int>(fragmentCheck));
        failure(line);
        return;
    }

    status("[PASS] GXM PROGRAM CHECK", 0xFF80E0FFu);

    const int vertexRegister = sceGxmShaderPatcherRegisterProgram(
        g_probeShaderPatcher, vertexProgram, &g_probeVertexProgramId);
    if (vertexRegister < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM VP REG 0X%08X",
                      static_cast<unsigned int>(vertexRegister));
        failure(line);
        return;
    }
    g_probeVertexRegistered = true;

    const int fragmentRegister = sceGxmShaderPatcherRegisterProgram(
        g_probeShaderPatcher, fragmentProgram, &g_probeFragmentProgramId);
    if (fragmentRegister < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM FP REG 0X%08X",
                      static_cast<unsigned int>(fragmentRegister));
        failure(line);
        return;
    }
    g_probeFragmentRegistered = true;

    status("[PASS] GXM PROGRAM REGISTER", 0xFF80E0FFu);

    const SceGxmProgramParameter* positionParam =
        sceGxmProgramFindParameterByName(vertexProgram, "aPosition");
    const SceGxmProgramParameter* colorParam =
        sceGxmProgramFindParameterByName(vertexProgram, "aColor");
    g_probeWvpParam =
        sceGxmProgramFindParameterByName(vertexProgram, "wvp");

    if (!positionParam || !colorParam || !g_probeWvpParam) {
        failure("[FAIL] GXM SHADER PARAMETERS");
        return;
    }

    status("[PASS] GXM SHADER PARAMETERS", 0xFF80E0FFu);

    SceGxmVertexAttribute attributes[2]{};

    attributes[0].streamIndex = 0;
    attributes[0].offset = 0;
    attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    attributes[0].componentCount = 3;
    attributes[0].regIndex =
        sceGxmProgramParameterGetResourceIndex(positionParam);

    attributes[1].streamIndex = 0;
    attributes[1].offset = 12;
    attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
    attributes[1].componentCount = 4;
    attributes[1].regIndex =
        sceGxmProgramParameterGetResourceIndex(colorParam);

    SceGxmVertexStream stream{};
    stream.stride = sizeof(azel::DebugColorVertex);
    stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    const int createVertexResult =
        sceGxmShaderPatcherCreateVertexProgram(
            g_probeShaderPatcher,
            g_probeVertexProgramId,
            attributes, 2,
            &stream, 1,
            &g_probeVertexProgram);

    if (createVertexResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM CREATE VP 0X%08X",
                      static_cast<unsigned int>(createVertexResult));
        failure(line);
        return;
    }

    status("[PASS] GXM CREATE VERTEX PROGRAM", 0xFF80E0FFu);

    const int createFragmentResult =
        sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_probeFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            vertexProgram,
            &g_probeFragmentProgram);

    if (createFragmentResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM CREATE FP 0X%08X",
                      static_cast<unsigned int>(createFragmentResult));
        failure(line);
        return;
    }

    status("[PASS] GXM CREATE FRAGMENT PROGRAM", 0xFF80E0FFu);

    SceGxmBlendInfo fadeBlend{};
    fadeBlend.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
    fadeBlend.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
    fadeBlend.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
    fadeBlend.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    fadeBlend.alphaSrc = SCE_GXM_BLEND_FACTOR_ZERO;
    fadeBlend.alphaDst = SCE_GXM_BLEND_FACTOR_ONE;
    fadeBlend.colorMask = SCE_GXM_COLOR_MASK_ALL;

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_probeFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            &fadeBlend,
            vertexProgram,
            &g_fadeFragmentProgram) < 0) {
        failure("[FAIL] CREATE FADE FP");
        return;
    }

    // Saturn VDP2 color offsets are signed additive RGB operations applied
    // after layer composition. Use GXM fixed-function blending directly:
    //   ADD              => dst + constant
    //   REVERSE_SUBTRACT => dst - constant
    // Alpha is preserved from the destination in both passes.
    SceGxmBlendInfo offsetAddBlend{};
    offsetAddBlend.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
    offsetAddBlend.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
    offsetAddBlend.colorSrc = SCE_GXM_BLEND_FACTOR_ONE;
    offsetAddBlend.colorDst = SCE_GXM_BLEND_FACTOR_ONE;
    offsetAddBlend.alphaSrc = SCE_GXM_BLEND_FACTOR_ZERO;
    offsetAddBlend.alphaDst = SCE_GXM_BLEND_FACTOR_ONE;
    offsetAddBlend.colorMask = SCE_GXM_COLOR_MASK_ALL;

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_probeFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            &offsetAddBlend,
            vertexProgram,
            &g_colorOffsetAddFragmentProgram) < 0) {
        failure("[FAIL] CREATE COLOR OFFSET ADD FP");
        return;
    }

    SceGxmBlendInfo offsetSubtractBlend = offsetAddBlend;
    offsetSubtractBlend.colorFunc =
        SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_probeFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            &offsetSubtractBlend,
            vertexProgram,
            &g_colorOffsetSubtractFragmentProgram) < 0) {
        failure("[FAIL] CREATE COLOR OFFSET SUB FP");
        return;
    }

    const SceGxmProgram* textureVertexGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_texture_v_gxp_start);
    const SceGxmProgram* textureFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_texture_f_gxp_start);
    const SceGxmProgram* meshFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_mesh_f_gxp_start);
    const SceGxmProgram* cinepakFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_cinepak_f_gxp_start);
    const SceGxmProgram* vdp2NbgFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_vdp2_nbg_f_gxp_start);
    const SceGxmProgram* vdp2Rbg0FragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_vdp2_rbg0_f_gxp_start);

    if (sceGxmProgramCheck(textureVertexGxp) < 0 ||
        sceGxmProgramCheck(textureFragmentGxp) < 0 ||
        sceGxmProgramCheck(meshFragmentGxp) < 0 ||
        sceGxmProgramCheck(cinepakFragmentGxp) < 0 ||
        sceGxmProgramCheck(vdp2NbgFragmentGxp) < 0) {
        failure("[FAIL] GXP CHECK");
        return;
    }

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            textureVertexGxp,
            &g_textureVertexProgramId) < 0) {
        failure("[FAIL] TEXTURE VP REG");
        return;
    }
    g_textureVertexRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            textureFragmentGxp,
            &g_textureFragmentProgramId) < 0) {
        failure("[FAIL] TEXTURE FP REG");
        return;
    }
    g_textureFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            cinepakFragmentGxp,
            &g_cinepakFragmentProgramId) < 0) {
        failure("[FAIL] CINEPAK FP REG");
        return;
    }
    g_cinepakFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            vdp2NbgFragmentGxp,
            &g_vdp2NbgFragmentProgramId) < 0) {
        failure("[FAIL] VDP2 NBG FP REG");
        return;
    }
    g_vdp2NbgFragmentRegistered = true;

    const int rbg0Check = sceGxmProgramCheck(vdp2Rbg0FragmentGxp);
    if (rbg0Check >= 0) {
        const int rbg0Register = sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            vdp2Rbg0FragmentGxp,
            &g_vdp2Rbg0FragmentProgramId);
        if (rbg0Register >= 0) {
            g_vdp2Rbg0FragmentRegistered = true;
            g_vdp2Rbg0Available = true;
        } else {
            logging::writef(
                "[NeptuneVDP2] RBG0 register unavailable=0x%08X; "
                "continuing with NBG/Cinepak\n",
                static_cast<unsigned int>(rbg0Register));
        }
    } else {
        logging::writef(
            "[NeptuneVDP2] RBG0 program check unavailable=0x%08X; "
            "continuing with NBG/Cinepak\n",
            static_cast<unsigned int>(rbg0Check));
    }

    g_cinepakMovieInfoParam =
        sceGxmProgramFindParameterByName(cinepakFragmentGxp, "movieInfo");
    if (!g_cinepakMovieInfoParam) {
        failure("[FAIL] CINEPAK SHADER PARAMS");
        return;
    }

    g_vdp2InfoParam =
        sceGxmProgramFindParameterByName(vdp2NbgFragmentGxp, "vdp2Info");
    if (!g_vdp2InfoParam) {
        failure("[FAIL] VDP2 NBG SHADER PARAMS");
        return;
    }

    if (g_vdp2Rbg0Available) {
        static const char* kRbg0PlaneNames[4] = {
            "rbg0Plane0", "rbg0Plane1", "rbg0Plane2", "rbg0Plane3"
        };
        for (unsigned int i = 0; i < 4u; ++i) {
            g_vdp2Rbg0PlaneParam[i] =
                sceGxmProgramFindParameterByName(
                    vdp2Rbg0FragmentGxp, kRbg0PlaneNames[i]);
            if (!g_vdp2Rbg0PlaneParam[i]) {
                logging::writef(
                    "[NeptuneVDP2] RBG0 plane uniform %u unavailable; "
                    "disabling RBG0\n", i);
                g_vdp2Rbg0Available = false;
                break;
            }
        }
        if (g_vdp2Rbg0Available) {
            g_vdp2Rbg0Transform0Param =
                sceGxmProgramFindParameterByName(
                    vdp2Rbg0FragmentGxp, "rbg0Transform0");
            g_vdp2Rbg0Transform1Param =
                sceGxmProgramFindParameterByName(
                    vdp2Rbg0FragmentGxp, "rbg0Transform1");
            g_vdp2Rbg0CoefficientParam =
                sceGxmProgramFindParameterByName(
                    vdp2Rbg0FragmentGxp, "rbg0Coefficient");
            g_vdp2Rbg0InfoParam =
                sceGxmProgramFindParameterByName(
                    vdp2Rbg0FragmentGxp, "rbg0Info");
            g_vdp2Rbg0FormatParam =
                sceGxmProgramFindParameterByName(
                    vdp2Rbg0FragmentGxp, "rbg0Format");
            if (!g_vdp2Rbg0Transform0Param ||
                !g_vdp2Rbg0Transform1Param ||
                !g_vdp2Rbg0CoefficientParam ||
                !g_vdp2Rbg0InfoParam ||
                !g_vdp2Rbg0FormatParam) {
                logging::writef(
                    "[NeptuneVDP2] RBG0 compact uniforms unavailable; "
                    "disabling RBG0\n");
                g_vdp2Rbg0Available = false;
            }
        }
    }

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            meshFragmentGxp,
            &g_meshFragmentProgramId) < 0) {
        failure("[FAIL] MESH FP REG");
        return;
    }
    g_meshFragmentRegistered = true;

    const SceGxmProgramParameter* texturePositionParam =
        sceGxmProgramFindParameterByName(
            textureVertexGxp, "aPosition");
    const SceGxmProgramParameter* textureUvParam =
        sceGxmProgramFindParameterByName(
            textureVertexGxp, "aTexcoord");
    g_textureWvpParam =
        sceGxmProgramFindParameterByName(textureVertexGxp, "wvp");

    if (!texturePositionParam || !textureUvParam ||
        !g_textureWvpParam) {
        failure("[FAIL] TEXTURE SHADER PARAMS");
        return;
    }

    SceGxmVertexAttribute textureAttributes[2]{};
    textureAttributes[0].streamIndex = 0;
    textureAttributes[0].offset = 0;
    textureAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    textureAttributes[0].componentCount = 3;
    textureAttributes[0].regIndex =
        sceGxmProgramParameterGetResourceIndex(texturePositionParam);

    textureAttributes[1].streamIndex = 0;
    textureAttributes[1].offset = 12;
    textureAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    textureAttributes[1].componentCount = 2;
    textureAttributes[1].regIndex =
        sceGxmProgramParameterGetResourceIndex(textureUvParam);

    SceGxmVertexStream textureStream{};
    textureStream.stride = sizeof(azel::DebugTextureVertex);
    textureStream.indexSource =
        SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    if (sceGxmShaderPatcherCreateVertexProgram(
            g_probeShaderPatcher,
            g_textureVertexProgramId,
            textureAttributes, 2,
            &textureStream, 1,
            &g_textureVertexProgram) < 0) {
        failure("[FAIL] CREATE TEXTURE VP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_textureFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            textureVertexGxp,
            &g_textureFragmentProgram) < 0) {
        failure("[FAIL] CREATE TEXTURE FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_textureFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            textureVertexGxp,
            &g_movieTextureFragmentProgram) < 0) {
        failure("[FAIL] CREATE MOVIE TEXTURE FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_cinepakFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            textureVertexGxp,
            &g_cinepakFragmentProgram) < 0) {
        failure("[FAIL] CREATE CINEPAK FP NO-MSAA");
        return;
    }

    if (g_vdp2Rbg0Available) {
        SceGxmBlendInfo rbg0Blend{};
        rbg0Blend.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
        rbg0Blend.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
        rbg0Blend.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
        rbg0Blend.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        rbg0Blend.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
        rbg0Blend.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        rbg0Blend.colorMask = SCE_GXM_COLOR_MASK_ALL;

        const int rbg0Create = sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_vdp2Rbg0FragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            &rbg0Blend,
            textureVertexGxp,
            &g_vdp2Rbg0FragmentProgram);
        if (rbg0Create < 0) {
            g_vdp2Rbg0FragmentProgram = nullptr;
            g_vdp2Rbg0Available = false;
            logging::writef(
                "[NeptuneVDP2] RBG0 fragment creation unavailable=0x%08X; "
                "continuing with NBG/Cinepak\n",
                static_cast<unsigned int>(rbg0Create));
        }
    }

    SceGxmBlendInfo vdp2NbgBlend{};
    vdp2NbgBlend.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
    vdp2NbgBlend.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
    vdp2NbgBlend.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
    vdp2NbgBlend.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    vdp2NbgBlend.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
    vdp2NbgBlend.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    vdp2NbgBlend.colorMask = SCE_GXM_COLOR_MASK_ALL;

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_vdp2NbgFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            &vdp2NbgBlend,
            textureVertexGxp,
            &g_vdp2NbgFragmentProgram) < 0) {
        failure("[FAIL] CREATE VDP2 NBG FP NO-MSAA");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_meshFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            textureVertexGxp,
            &g_meshTextureFragmentProgram) < 0) {
        failure("[FAIL] CREATE MESH TEXTURE FP");
        return;
    }

    const SceGxmProgram* gouraudPayloadVertexGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_payload_v_gxp_start);
    const SceGxmProgram* gouraudSubdivVertexGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_subdiv_v_gxp_start);
    const SceGxmProgram* texturedGouraudSubdivFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_gouraud_subdiv_f_gxp_start);
    const SceGxmProgram* gouraudSubdivGrayFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_subdiv_gray_f_gxp_start);
    const SceGxmProgram* gouraudDebugFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_debug_f_gxp_start);
    const SceGxmProgram* texturedLitFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_lit_f_gxp_start);
    const SceGxmProgram* texturedLitOpaqueFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_lit_opaque_f_gxp_start);
    const SceGxmProgram* texturedLitHalfFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_lit_half_f_gxp_start);
    const SceGxmProgram* texturedLitNewtonFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_lit_newton_f_gxp_start);
    const SceGxmProgram* texturedPayloadProbeFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_payload_probe_f_gxp_start);
    const SceGxmProgram* texturedGouraudNoInverseFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_gouraud_noinverse_f_gxp_start);
    const SceGxmProgram* texturedGouraudNoQuantFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_gouraud_noquant_f_gxp_start);
    const SceGxmProgram* texturedRgb555NoGouraudFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_rgb555_nogouraud_f_gxp_start);
    const SceGxmProgram* texturedGouraudFinalQuantFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_gouraud_finalquant_f_gxp_start);
    const SceGxmProgram* texturedGouraudScanlineFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_gouraud_scanline_f_gxp_start);
    const SceGxmProgram* gouraudScanlineGrayFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_scanline_gray_f_gxp_start);

    if (sceGxmProgramCheck(gouraudPayloadVertexGxp) < 0 ||
        sceGxmProgramCheck(gouraudSubdivVertexGxp) < 0 ||
        sceGxmProgramCheck(texturedGouraudSubdivFragmentGxp) < 0 ||
        sceGxmProgramCheck(gouraudSubdivGrayFragmentGxp) < 0 ||
        sceGxmProgramCheck(gouraudDebugFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedLitFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedLitOpaqueFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedLitHalfFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedLitNewtonFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedPayloadProbeFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedGouraudNoInverseFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedGouraudNoQuantFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedRgb555NoGouraudFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedGouraudFinalQuantFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedGouraudScanlineFragmentGxp) < 0 ||
        sceGxmProgramCheck(gouraudScanlineGrayFragmentGxp) < 0) {
        failure("[FAIL] GOURAUD PAYLOAD GXP CHECK");
        return;
    }

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            gouraudPayloadVertexGxp,
            &g_gouraudPayloadVertexProgramId) < 0) {
        failure("[FAIL] GOURAUD PAYLOAD VP REG");
        return;
    }
    g_gouraudPayloadVertexRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            gouraudSubdivVertexGxp,
            &g_gouraudSubdivVertexProgramId) < 0) {
        failure("[FAIL] GOURAUD SUBDIV VP REG");
        return;
    }
    g_gouraudSubdivVertexRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedGouraudSubdivFragmentGxp,
            &g_texturedGouraudSubdivFragmentProgramId) < 0) {
        failure("[FAIL] GOURAUD SUBDIV FP REG");
        return;
    }
    g_texturedGouraudSubdivFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            gouraudSubdivGrayFragmentGxp,
            &g_gouraudSubdivGrayFragmentProgramId) < 0) {
        failure("[FAIL] GOURAUD SUBDIV GRAY FP REG");
        return;
    }
    g_gouraudSubdivGrayFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            gouraudDebugFragmentGxp,
            &g_gouraudDebugFragmentProgramId) < 0) {
        failure("[FAIL] GOURAUD DEBUG FP REG");
        return;
    }
    g_gouraudDebugFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedLitFragmentGxp,
            &g_texturedLitFragmentProgramId) < 0) {
        failure("[FAIL] TEXTURED LIT PROGRAM REG");
        return;
    }
    g_texturedLitFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedLitOpaqueFragmentGxp,
            &g_texturedLitOpaqueFragmentProgramId) < 0) {
        failure("[FAIL] OPAQUE TEXTURED LIT PROGRAM REG");
        return;
    }
    g_texturedLitOpaqueFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedLitHalfFragmentGxp,
            &g_texturedLitHalfFragmentProgramId) < 0) {
        failure("[FAIL] HALF TEXTURED LIT PROGRAM REG");
        return;
    }
    g_texturedLitHalfFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedLitNewtonFragmentGxp,
            &g_texturedLitNewtonFragmentProgramId) < 0) {
        failure("[FAIL] NEWTON LIT PROGRAM REG");
        return;
    }
    g_texturedLitNewtonFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedPayloadProbeFragmentGxp,
            &g_texturedPayloadProbeFragmentProgramId) < 0) {
        failure("[FAIL] PAYLOAD PROBE PROGRAM REG");
        return;
    }
    g_texturedPayloadProbeFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedGouraudNoInverseFragmentGxp,
            &g_texturedGouraudNoInverseFragmentProgramId) < 0) {
        failure("[FAIL] NO-INVERSE GOURAUD PROGRAM REG");
        return;
    }
    g_texturedGouraudNoInverseFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedGouraudNoQuantFragmentGxp,
            &g_texturedGouraudNoQuantFragmentProgramId) < 0) {
        failure("[FAIL] NO-QUANT GOURAUD PROGRAM REG");
        return;
    }
    g_texturedGouraudNoQuantFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedRgb555NoGouraudFragmentGxp,
            &g_texturedRgb555NoGouraudFragmentProgramId) < 0) {
        failure("[FAIL] RGB555 NO-GOURAUD PROGRAM REG");
        return;
    }
    g_texturedRgb555NoGouraudFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedGouraudFinalQuantFragmentGxp,
            &g_texturedGouraudFinalQuantFragmentProgramId) < 0) {
        failure("[FAIL] FINAL-QUANT GOURAUD PROGRAM REG");
        return;
    }
    g_texturedGouraudFinalQuantFragmentRegistered = true;

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedGouraudScanlineFragmentGxp,
            &g_texturedGouraudScanlineFragmentProgramId) < 0) {
        failure("[FAIL] SCANLINE GOURAUD PROGRAM REG");
        return;
    }
    g_texturedGouraudScanlineFragmentRegistered = true;
    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            gouraudScanlineGrayFragmentGxp,
            &g_gouraudScanlineGrayFragmentProgramId) < 0) {
        failure("[FAIL] SCANLINE GRAY PROGRAM REG");
        return;
    }
    g_gouraudScanlineGrayFragmentRegistered = true;


    const SceGxmProgramParameter* subdivPositionParam =
        sceGxmProgramFindParameterByName(gouraudSubdivVertexGxp, "aPosition");
    const SceGxmProgramParameter* subdivUvParam =
        sceGxmProgramFindParameterByName(gouraudSubdivVertexGxp, "aTexcoord");
    const SceGxmProgramParameter* subdivShadeParam =
        sceGxmProgramFindParameterByName(gouraudSubdivVertexGxp, "aShade");
    g_gouraudSubdivWvpParam =
        sceGxmProgramFindParameterByName(gouraudSubdivVertexGxp, "wvp");
    if (!subdivPositionParam || !subdivUvParam || !subdivShadeParam ||
        !g_gouraudSubdivWvpParam) {
        failure("[FAIL] GOURAUD SUBDIV VP PARAMS");
        return;
    }

    SceGxmVertexAttribute subdivAttributes[3]{};
    const SceGxmProgramParameter* subdivParams[3] = {
        subdivPositionParam, subdivUvParam, subdivShadeParam
    };
    const unsigned int subdivOffsets[3] = {0u, 12u, 20u};
    const unsigned int subdivComponents[3] = {3u, 2u, 3u};
    for (unsigned int i = 0; i < 3u; ++i) {
        subdivAttributes[i].streamIndex = 0;
        subdivAttributes[i].offset = subdivOffsets[i];
        subdivAttributes[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
        subdivAttributes[i].componentCount = subdivComponents[i];
        subdivAttributes[i].regIndex =
            sceGxmProgramParameterGetResourceIndex(subdivParams[i]);
    }
    SceGxmVertexStream subdivStream{};
    subdivStream.stride = sizeof(SubdivGouraudVertex);
    subdivStream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
    if (sceGxmShaderPatcherCreateVertexProgram(
            g_probeShaderPatcher,
            g_gouraudSubdivVertexProgramId,
            subdivAttributes, 3,
            &subdivStream, 1,
            &g_gouraudSubdivVertexProgram) < 0) {
        failure("[FAIL] CREATE GOURAUD SUBDIV VP");
        return;
    }
    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedGouraudSubdivFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudSubdivVertexGxp,
            &g_texturedGouraudSubdivFragmentProgram) < 0) {
        failure("[FAIL] CREATE GOURAUD SUBDIV FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_meshFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudSubdivVertexGxp,
            &g_meshSubdivFragmentProgram) < 0) {
        failure("[FAIL] CREATE MESH SUBDIV FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_gouraudSubdivGrayFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudSubdivVertexGxp,
            &g_gouraudSubdivGrayFragmentProgram) < 0) {
        failure("[FAIL] CREATE GOURAUD SUBDIV GRAY FP");
        return;
    }

    const SceGxmProgramParameter* gouraudPositionParam =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aPosition");
    const SceGxmProgramParameter* gouraudUvParam =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aTexcoord");
    const SceGxmProgramParameter* gouraudQuad01Param =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aQuadScreen01");
    const SceGxmProgramParameter* gouraudQuad23Param =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aQuadScreen23");
    const SceGxmProgramParameter* gouraudRParam =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aGouraudR");
    const SceGxmProgramParameter* gouraudGParam =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aGouraudG");
    const SceGxmProgramParameter* gouraudBParam =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "aGouraudB");
    g_gouraudPayloadWvpParam =
        sceGxmProgramFindParameterByName(
            gouraudPayloadVertexGxp, "wvp");

    if (!gouraudPositionParam ||
        !gouraudUvParam ||
        !gouraudQuad01Param ||
        !gouraudQuad23Param ||
        !gouraudRParam ||
        !gouraudGParam ||
        !gouraudBParam ||
        !g_gouraudPayloadWvpParam) {
        failure("[FAIL] GOURAUD PAYLOAD VP PARAMS");
        return;
    }

    SceGxmVertexAttribute gouraudAttributes[7]{};
    const SceGxmProgramParameter* gouraudParams[7] = {
        gouraudPositionParam,
        gouraudUvParam,
        gouraudQuad01Param,
        gouraudQuad23Param,
        gouraudRParam,
        gouraudGParam,
        gouraudBParam
    };
    const unsigned int gouraudOffsets[7] = {
        0u, 12u, 20u, 36u, 52u, 68u, 84u
    };
    const unsigned int gouraudComponents[7] = {
        3u, 2u, 4u, 4u, 4u, 4u, 4u
    };

    for (unsigned int i = 0; i < 7u; ++i) {
        gouraudAttributes[i].streamIndex = 0;
        gouraudAttributes[i].offset = gouraudOffsets[i];
        gouraudAttributes[i].format =
            SCE_GXM_ATTRIBUTE_FORMAT_F32;
        gouraudAttributes[i].componentCount =
            gouraudComponents[i];
        gouraudAttributes[i].regIndex =
            sceGxmProgramParameterGetResourceIndex(
                gouraudParams[i]);
    }

    SceGxmVertexStream gouraudStream{};
    gouraudStream.stride =
        sizeof(azel::DebugGouraudPayloadVertex);
    gouraudStream.indexSource =
        SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    if (sceGxmShaderPatcherCreateVertexProgram(
            g_probeShaderPatcher,
            g_gouraudPayloadVertexProgramId,
            gouraudAttributes, 7,
            &gouraudStream, 1,
            &g_gouraudPayloadVertexProgram) < 0) {
        failure("[FAIL] CREATE GOURAUD PAYLOAD VP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_gouraudDebugFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_gouraudDebugFragmentProgram) < 0) {
        failure("[FAIL] CREATE GOURAUD DEBUG FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedLitFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedLitFragmentProgram) < 0) {
        failure("[FAIL] CREATE TEXTURED LIT FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedLitOpaqueFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedLitOpaqueFragmentProgram) < 0) {
        failure("[FAIL] CREATE OPAQUE TEXTURED LIT FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedLitHalfFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedLitHalfFragmentProgram) < 0) {
        failure("[FAIL] CREATE HALF TEXTURED LIT FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedLitNewtonFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedLitNewtonFragmentProgram) < 0) {
        failure("[FAIL] CREATE NEWTON LIT FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedPayloadProbeFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedPayloadProbeFragmentProgram) < 0) {
        failure("[FAIL] CREATE PAYLOAD PROBE FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedGouraudNoInverseFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedGouraudNoInverseFragmentProgram) < 0) {
        failure("[FAIL] CREATE NO-INVERSE GOURAUD FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedGouraudNoQuantFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedGouraudNoQuantFragmentProgram) < 0) {
        failure("[FAIL] CREATE NO-QUANT GOURAUD FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedRgb555NoGouraudFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedRgb555NoGouraudFragmentProgram) < 0) {
        failure("[FAIL] CREATE RGB555 NO-GOURAUD FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedGouraudFinalQuantFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedGouraudFinalQuantFragmentProgram) < 0) {
        failure("[FAIL] CREATE FINAL-QUANT GOURAUD FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedGouraudScanlineFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedGouraudScanlineFragmentProgram) < 0) {
        failure("[FAIL] CREATE SCANLINE GOURAUD FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_gouraudScanlineGrayFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            kMultisampleMode,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_gouraudScanlineGrayFragmentProgram) < 0) {
        failure("[FAIL] CREATE SCANLINE GRAY FP");
        return;
    }

    status("[PASS] GXM GOURAUD VERTEX PAYLOAD", 0xFF80E0FFu);
    status("[PASS] GXM GOURAUD SUBDIV PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM GOURAUD SUBDIV GRAY", 0xFF80E0FFu);
    status("[PASS] GXM TEXTURED LIGHTING PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM OPAQUE LIGHTING PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM HALF EXACT LIGHTING PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM NEWTON LIGHTING PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM PAYLOAD PROBE PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM NO-INVERSE GOURAUD PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM NO-QUANT GOURAUD PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM RGB555 NO-GOURAUD PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM FINAL-QUANT GOURAUD PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM SCANLINE GOURAUD PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM SCANLINE GRAY PIPELINE", 0xFF80E0FFu);
    status("[PASS] GXM TEXTURE PIPELINE", 0xFF80E0FFu);

    // Stage 9: begin/end one empty scene. Bind the patched programs and
    // conservative default state, but submit no vertex/index buffers and
    // issue no draw call.
    std::memset(g_probeDepth, 0xFF,
                ((kWidth + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1)) *
                ((kHeight + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1)) * 4u);
    std::memset(g_probeStencil, 0,
                ((kWidth + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1)) *
                ((kHeight + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1)) * 4u);

    g_fadeVertices = static_cast<azel::DebugColorVertex*>(
        probeGpuAlloc(
            6u * sizeof(azel::DebugColorVertex),
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_fadeVertexUid));
    g_fadeIndices = static_cast<std::uint16_t*>(
        probeGpuAlloc(
            6u * sizeof(std::uint16_t),
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_fadeIndexUid));
    if (!g_fadeVertices || !g_fadeIndices) {
        failure("[FAIL] FADE GPU BUFFER");
        return;
    }
    static const float xy[6][2] = {
        {-1.0f,-1.0f}, { 1.0f,-1.0f}, { 1.0f, 1.0f},
        {-1.0f,-1.0f}, { 1.0f, 1.0f}, {-1.0f, 1.0f}
    };
    for (unsigned i = 0; i < 6u; ++i) {
        g_fadeVertices[i].x = xy[i][0];
        g_fadeVertices[i].y = xy[i][1];
        g_fadeVertices[i].z = 0.0f;
        g_fadeVertices[i].r = 0u;
        g_fadeVertices[i].g = 0u;
        g_fadeVertices[i].b = 0u;
        g_fadeVertices[i].a = 255u;
        g_fadeIndices[i] = static_cast<std::uint16_t>(i);
    }

    const int beginResult = sceGxmBeginScene(
        g_probeContext,
        0,
        g_probeRenderTarget,
        nullptr,
        nullptr,
        g_probeSync,
        &g_probeColorSurface,
        &g_probeDepthSurface);

    if (beginResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM BEGIN SCENE 0X%08X",
                      static_cast<unsigned int>(beginResult));
        failure(line);
        return;
    }

    status("[PASS] GXM BEGIN SCENE", 0xFF80E0FFu);

    sceGxmSetVertexProgram(g_probeContext, g_probeVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext, g_probeFragmentProgram);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetDefaultRegionClipAndViewport(
        g_probeContext, kWidth - 1, kHeight - 1);
    sceGxmSetFrontDepthFunc(
        g_probeContext, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetBackDepthFunc(
        g_probeContext, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);

    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    sceGxmFinish(g_probeContext);

    g_probeScenePassed = true;
    status("[PASS] GXM END SCENE", 0xFF80E0FFu);

    // Basic Wing is retained only as an optional renderer regression path.
    // Native 0.040 boot can enter the title movie before that debug asset has
    // been prepared, so it must not gate display ownership or movie output.
    if (g_basicWingCpuReady && !g_basicWingCpuMesh.vertices.empty()) {
        const Vdp1ModelSource vdp1Source = basicWingVdp1Source();
        if (prepare_vdp1_model(vdp1Source)) {
            g_residentVdp1Model = ResidentVdp1Model::BasicWing;
            status("[PASS] GXM BASIC WING VDP1 PREPARE", 0xFF80E0FFu);
        } else {
            status("[INFO] BASIC WING VDP1 REGRESSION PATH UNAVAILABLE",
                   0xFFB0B0B0u);
        }
    }

    std::memset(
        g_probeColorBuffer,
        0,
        static_cast<std::size_t>(gxmPitch) * kHeight *
            sizeof(std::uint32_t));

    SceDisplayFrameBuf gxmFb{};
    gxmFb.size = sizeof(gxmFb);
    gxmFb.base = g_probeColorBuffer;
    gxmFb.pitch = gxmPitch;
    gxmFb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    gxmFb.width = kWidth;
    gxmFb.height = kHeight;

    const int displayResult =
        sceDisplaySetFrameBuf(&gxmFb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (displayResult < 0) {
        char line[78];
        std::snprintf(
            line, sizeof(line),
            "[FAIL] GXM DISPLAY 0X%08X",
            static_cast<unsigned int>(displayResult));
        failure(line);
        return;
    }

    sceDisplayWaitVblankStart();

    // Renderer readiness belongs to the completed Neptune/GXM backend, not to
    // any optional regression asset. Authentic boot reaches native scenes
    // without ever calling load_basic_wing_viewer().
    g_viewerReady = true;
    g_probeDisplayingGxm = true;
    g_gxmDrawBuffer = 1;
    g_debugVisible = false;
    g_suppressGxmInitPassStatus = false;
    status("[PASS] GXM INITIALIZATION + VDP1 READY", 0xFF80E0FFu);
    std::printf("[GXM] black town boot framebuffer queued for display\n");
}

bool debug_console_visible()
{
    return g_debugVisible;
}

bool load_edge_idle_model(azel::BasicWingDebugMesh&& mesh)
{
    if (mesh.vertices.empty() ||
        mesh.vertices.size() != mesh.polygons * 6u ||
        mesh.polygonRecords.size() != mesh.polygons ||
        mesh.gouraud555.size() != mesh.polygons ||
        !mesh.mode1DecodeFullyResolved)
        return false;

    const unsigned models = mesh.models;
    const unsigned polygons = mesh.polygons;
    const unsigned decodedTextures = mesh.decodedTextures;
    g_edgeIdleCpuMesh = std::move(mesh);
    g_edgeIdleCpuReady = true;

    char line[78];
    std::snprintf(
        line, sizeof(line),
        "[PASS] EDGE IDLE %u MODELS / %u POLYS / %u TEX",
        models, polygons, decodedTextures);
    status(line, 0xFF70E0A0u);
    return true;
}

bool load_static_room_viewer(const azel::StaticRoomDebugMesh& mesh)
{
    if (mesh.vertices.empty() ||
        mesh.vertices.size() != mesh.polygons * 6u ||
        mesh.polygonRecords.size() != mesh.polygons ||
        mesh.gouraud555.size() != mesh.polygons)
        return false;

    g_staticRoomCpuMesh = mesh;
    g_staticRoomCpuReady = true;
    g_viewMode = 7; // Full
    g_pendingViewMode = 7;

    g_townPlayerReady =
        mesh.cameraValid &&
        mesh.edgeTransformValid;
    if (g_townPlayerReady) {
        std::memcpy(
            g_townPlayerPosition,
            mesh.edgePosition,
            sizeof(g_townPlayerPosition));

        // edgeRotation[] is stored in turns. Y is the town heading.
        constexpr float kTau =
            6.28318530717958647692f;
        g_townPlayerYaw =
            mesh.edgeRotation[1] * kTau;
        std::memcpy(
            g_townCameraPosition,
            mesh.cameraPosition,
            sizeof(g_townCameraPosition));
        std::memcpy(
            g_townCameraRawPosition,
            mesh.cameraPosition,
            sizeof(g_townCameraRawPosition));
        std::memcpy(
            g_townCameraTarget,
            mesh.cameraTarget,
            sizeof(g_townCameraTarget));
        std::memcpy(
            g_townCameraUp,
            mesh.cameraUp,
            sizeof(g_townCameraUp));

        const float anchorY =
            mesh.edgePosition[1] +
            static_cast<float>(0x1800) / 65536.0f;
        const float ox =
            mesh.cameraPosition[0] - mesh.edgePosition[0];
        const float oy =
            mesh.cameraPosition[1] - anchorY;
        const float oz =
            mesh.cameraPosition[2] - mesh.edgePosition[2];
        g_townCameraDistance =
            std::sqrt(ox*ox + oy*oy + oz*oz);
        g_townCameraYaw = std::atan2(ox, oz);
        g_townCameraPitch =
            std::atan2(oy, std::sqrt(ox*ox + oz*oz));

        std::memcpy(
            g_pendingTownPlayerPosition,
            g_townPlayerPosition,
            sizeof(g_townPlayerPosition));
        g_pendingTownPlayerYaw = g_townPlayerYaw;
        std::memcpy(
            g_pendingTownCameraPosition,
            g_townCameraPosition,
            sizeof(g_townCameraPosition));
        std::memcpy(
            g_pendingTownCameraRawPosition,
            g_townCameraRawPosition,
            sizeof(g_townCameraRawPosition));
        std::memcpy(
            g_pendingTownCameraTarget,
            g_townCameraTarget,
            sizeof(g_townCameraTarget));
        std::memcpy(
            g_pendingTownCameraUp,
            g_townCameraUp,
            sizeof(g_townCameraUp));
        g_pendingTownCameraYaw = g_townCameraYaw;
        g_pendingTownCameraPitch = g_townCameraPitch;
        g_pendingTownCameraDistance = g_townCameraDistance;
        g_pendingTownPresentationValid = true;

        status(
            "[PASS] RUIN LIVE EDGE/FOLLOW STATE",
            0xFF70E0A0u);
    }

    if (g_edgeIdleCpuReady) {
        g_edgeFirstVertex =
            g_staticRoomCpuMesh.worldVertices.size();
        g_edgeFirstPolygon =
            g_staticRoomCpuMesh.polygonRecords.size();

        const std::uint16_t textureBase =
            static_cast<std::uint16_t>(
                g_staticRoomCpuMesh.decodedTextureData.size());

        g_staticRoomCpuMesh.worldVertices.insert(
            g_staticRoomCpuMesh.worldVertices.end(),
            g_edgeIdleCpuMesh.vertices.begin(),
            g_edgeIdleCpuMesh.vertices.end());
        g_staticRoomCpuMesh.worldLightingVertices.insert(
            g_staticRoomCpuMesh.worldLightingVertices.end(),
            g_edgeIdleCpuMesh.lightingVertices.begin(),
            g_edgeIdleCpuMesh.lightingVertices.end());
        g_staticRoomCpuMesh.vertices.insert(
            g_staticRoomCpuMesh.vertices.end(),
            g_edgeIdleCpuMesh.vertices.begin(),
            g_edgeIdleCpuMesh.vertices.end());
        g_staticRoomCpuMesh.lightingVertices.insert(
            g_staticRoomCpuMesh.lightingVertices.end(),
            g_edgeIdleCpuMesh.lightingVertices.begin(),
            g_edgeIdleCpuMesh.lightingVertices.end());
        g_staticRoomCpuMesh.polygonRecords.insert(
            g_staticRoomCpuMesh.polygonRecords.end(),
            g_edgeIdleCpuMesh.polygonRecords.begin(),
            g_edgeIdleCpuMesh.polygonRecords.end());
        g_staticRoomCpuMesh.gouraud555.insert(
            g_staticRoomCpuMesh.gouraud555.end(),
            g_edgeIdleCpuMesh.gouraud555.begin(),
            g_edgeIdleCpuMesh.gouraud555.end());
        g_staticRoomCpuMesh.decodedTextureData.insert(
            g_staticRoomCpuMesh.decodedTextureData.end(),
            g_edgeIdleCpuMesh.decodedTextureData.begin(),
            g_edgeIdleCpuMesh.decodedTextureData.end());

        for (const auto index : g_edgeIdleCpuMesh.polygonTextureIndices) {
            g_staticRoomCpuMesh.polygonTextureIndices.push_back(
                static_cast<std::uint16_t>(textureBase + index));
        }

        g_edgeShadowTownTextureIndices.clear();
        if (g_edgeShadowCpuReady &&
            !g_edgeShadowCpuMesh.decodedTextureData.empty()) {
            const std::uint16_t shadowTextureBase =
                static_cast<std::uint16_t>(
                    g_staticRoomCpuMesh.decodedTextureData.size());
            g_staticRoomCpuMesh.decodedTextureData.insert(
                g_staticRoomCpuMesh.decodedTextureData.end(),
                g_edgeShadowCpuMesh.decodedTextureData.begin(),
                g_edgeShadowCpuMesh.decodedTextureData.end());
            g_edgeShadowTownTextureIndices.reserve(
                g_edgeShadowCpuMesh.polygonTextureIndices.size());
            for (const auto index :
                 g_edgeShadowCpuMesh.polygonTextureIndices) {
                g_edgeShadowTownTextureIndices.push_back(
                    static_cast<std::uint16_t>(
                        shadowTextureBase + index));
            }
        }

        g_staticRoomCpuMesh.polygons +=
            g_edgeIdleCpuMesh.polygons;
        g_staticRoomCpuMesh.models +=
            g_edgeIdleCpuMesh.models;
        g_staticRoomCpuMesh.decodedTextures =
            static_cast<unsigned>(
                g_staticRoomCpuMesh.decodedTextureData.size());

        transformTownEdgeVertices();
        status(
            "[PASS] EDGE VISUAL MERGED INTO RUIN",
            0xFF70E0A0u);
    }

    computeDebugFrame(
        g_staticRoomCpuMesh.vertices,
        g_staticRoomViewCenter,
        g_staticRoomFitDistance);

    char line[78];
    std::snprintf(
        line, sizeof(line),
        "[PASS] RUIN ROOM %u OBJ / %u POLYS",
        mesh.objects,
        mesh.polygons);
    status(line, 0xFF70E0A0u);

    if (mesh.texturesFullyResolved) {
        char textureLine[78];
        std::snprintf(
            textureLine, sizeof(textureLine),
            "[PASS] RUIN ROOM %u TEXTURES",
            mesh.decodedTextures);
        status(textureLine, 0xFF80E0FFu);

        if (mesh.lightingValid) {
            status(
                "[PASS] RUIN SCENE LIGHTING DATA",
                0xFF80E0FFu);
        }
        if (mesh.cameraValid) {
            status(
                "[PASS] RUIN INITIAL TOWN CAMERA",
                0xFF80E0FFu);
        }
        if (!mesh.objectStates.empty() &&
            mesh.cellRadius > 0.0f) {
            status(
                "[PASS] RUIN AZEL GRID CULL + LOD",
                0xFF80E0FFu);
        }
    } else if (mesh.texturesValid) {
        status("[INFO] RUIN ROOM TEXTURES NEED CRAM", 0xFF80C0FFu);
    } else {
        status("[INFO] RUIN ROOM POLYGON COLOR FALLBACK", 0xFFB0B0B0u);
    }

    return true;
}

bool load_basic_wing_viewer()
{
    g_basicWingCpuMesh = {};
    if (!azel::build_basic_wing_debug_mesh(g_basicWingCpuMesh) ||
        g_basicWingCpuMesh.vertices.empty() ||
        g_basicWingCpuMesh.lightingVertices.size() !=
            g_basicWingCpuMesh.vertices.size() ||
        g_basicWingCpuMesh.vertices.size() > 65535) {
        g_basicWingCpuReady = false;
        g_viewerReady = false;
        return false;
    }

    if (g_basicWingCpuMesh.polygonRecords.size() !=
        g_basicWingCpuMesh.polygons) {
        g_basicWingCpuReady = false;
        g_viewerReady = false;
        return false;
    }

    // Basic Wing vertices and every animation frame remain in native Azel
    // game space. The debug viewer fits the model by camera distance only.
    computeDebugFrame(
        g_basicWingCpuMesh.vertices,
        g_basicWingViewCenter,
        g_basicWingFitDistance);

    g_viewYaw = 0.60f;
    g_viewPitch = -0.30f;
    g_viewDistance = g_basicWingFitDistance;
    g_viewMode = 0;
    g_basicWingAnimationFrame = 0;
    g_presentClockInitialized = false;
    g_lastPresentVcount = 0;
    g_basicWingCpuReady = true;
    g_viewerReady = true;

    lagi::platform::logging::writef(
        "[Viewer] Basic Wing game-space center=(%.5f,%.5f,%.5f) fit=%.5f AzelFOV=80\n",
        g_basicWingViewCenter[0],
        g_basicWingViewCenter[1],
        g_basicWingViewCenter[2],
        g_basicWingFitDistance);

    status("[PASS] DRAGON0 VDP1 212 RECORDS", 0xFF80E0FFu);
    if (g_basicWingCpuMesh.lightingPayloadValid)
        status("[PASS] DRAGON0 LIGHTING DATA", 0xFF80E0FFu);
    else
        failure("[FAIL] DRAGON0 LIGHTING DATA");

    if (g_basicWingCpuMesh.cgbReferencesValid)
        status("[PASS] DRAGON0 CGB REFERENCES", 0xFF80E0FFu);
    else
        failure("[FAIL] DRAGON0 CGB REFERENCES");

    if (g_basicWingCpuMesh.animationValid) {
        status("[PASS] DRAGON0 MORPH FLAP ANIM", 0xFF80E0FFu);
    } else {
        status("[INFO] DRAGON0 MORPH ANIM STATIC", 0xFF80C0FFu);
    }

    if (g_basicWingCpuMesh.mode1DecodeFullyResolved) {
        status("[PASS] DRAGON0 MODE1 TEXTURES", 0xFF80E0FFu);
    } else if (g_basicWingCpuMesh.mode1DecodeValid) {
        status("[INFO] DRAGON0 MODE1 NEEDS CRAM", 0xFF80C0FFu);
    } else {
        failure("[FAIL] DRAGON0 MODE1 DECODE");
    }
    return true;
}

bool load_edge_shadow_model(azel::BasicWingDebugMesh&& mesh)
{
    if (mesh.vertices.empty() ||
        mesh.vertices.size() != mesh.polygons * 6u ||
        mesh.polygonRecords.size() != mesh.polygons ||
        !mesh.mode1DecodeFullyResolved ||
        mesh.polygonTextureIndices.size() != mesh.polygons)
        return false;

    g_edgeShadowCpuMesh = std::move(mesh);
    g_edgeShadowCpuReady = true;

    char line[78];
    std::snprintf(
        line, sizeof(line),
        "[PASS] EDGE SHADOW %u POLYS / VDP1 MESH",
        g_edgeShadowCpuMesh.polygons);
    status(line, 0xFF70E0A0u);
    return true;
}


bool submit_vdp1_model(
    const Vdp1ModelSource& model,
    const Vdp1DrawState& drawState)
{
    if (!model.valid() || !g_probeContext ||
        !g_vdp1Vertices || !g_vdp1LightingVertices ||
        !g_vdp1Indices)
        return false;

    const bool textured =
        drawState.mode == Vdp1RenderMode::Textured &&
        g_vdp1TexturedReady && model.texturesValid() &&
        g_vdp1TextureVertices && g_vdp1TextureIndices;
    const bool texturedLit =
        drawState.mode == Vdp1RenderMode::TexturedGouraud &&
        g_vdp1TexturedReady && model.texturesValid() &&
        g_vdp1GouraudVertices && g_vdp1TextureIndices;
    const bool gouraudGray =
        drawState.mode == Vdp1RenderMode::GouraudGrayscale &&
        g_vdp1GouraudVertices && g_vdp1TextureIndices;
    const bool wireframe =
        drawState.mode == Vdp1RenderMode::Wireframe;
    const bool subdividedTexturedLit =
        texturedLit &&
        g_viewMode == 7 &&
        g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
        g_vdp1SubdivVertices && g_vdp1SubdivIndices &&
        g_vdp1SubdivVertexCapacity >= model.polygonCount * 9u &&
        g_vdp1SubdivIndexCapacity >= model.polygonCount * 24u &&
        g_gouraudSubdivVertexProgram &&
        g_texturedGouraudSubdivFragmentProgram;
    const bool subdividedGouraudGray =
        gouraudGray &&
        g_viewMode == 10 &&
        g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
        g_vdp1SubdivVertices && g_vdp1SubdivIndices &&
        g_vdp1SubdivVertexCapacity >= model.polygonCount * 9u &&
        g_vdp1SubdivIndexCapacity >= model.polygonCount * 24u &&
        g_gouraudSubdivVertexProgram &&
        g_gouraudSubdivGrayFragmentProgram;
    const bool subdividedGouraud =
        subdividedTexturedLit || subdividedGouraudGray;
    const bool gouraudPath =
        texturedLit || gouraudGray;

    sceGxmSetVertexProgram(
        g_probeContext,
        subdividedGouraud
            ? g_gouraudSubdivVertexProgram
            : (gouraudPath
            ? g_gouraudPayloadVertexProgram
            : (textured
                ? g_textureVertexProgram
                : g_probeVertexProgram)));

    sceGxmSetFragmentProgram(
        g_probeContext,
        subdividedGouraud
            ? (subdividedTexturedLit
                ? g_texturedGouraudSubdivFragmentProgram
                : g_gouraudSubdivGrayFragmentProgram)
            : (texturedLit
            // Same edge selection and RGB555 result as the reference path;
            // shade/color arithmetic is FP16 to reduce SGX fragment pressure.
            ? g_texturedLitHalfFragmentProgram
            : (gouraudGray
                ? g_gouraudDebugFragmentProgram
                : (textured
                    ? g_textureFragmentProgram
                    : g_probeFragmentProgram))));

    // Winding compensation follows the projection actually used by the
    // current draw state. Field mode no longer receives the town X mirror,
    // so it does not need an extra rasterizer-side inversion.
    const SceGxmCullMode cullMode =
        wireframe
            ? SCE_GXM_CULL_NONE
            : (drawState.reverseCullWinding
                ? SCE_GXM_CULL_CCW
                : SCE_GXM_CULL_CW);
    sceGxmSetCullMode(g_probeContext, cullMode);
    sceGxmSetDefaultRegionClipAndViewport(
        g_probeContext,
        viewerRenderWidth() - 1,
        viewerRenderHeight() - 1);

    const SceGxmDepthFunc depthFunc =
        wireframe ? SCE_GXM_DEPTH_FUNC_LESS
                  : SCE_GXM_DEPTH_FUNC_LESS_EQUAL;
    sceGxmSetFrontDepthFunc(g_probeContext, depthFunc);
    sceGxmSetBackDepthFunc(g_probeContext, depthFunc);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);

    const SceGxmPolygonMode polygonMode =
        wireframe ? SCE_GXM_POLYGON_MODE_LINE
                  : SCE_GXM_POLYGON_MODE_TRIANGLE_FILL;
    sceGxmSetFrontPolygonMode(g_probeContext, polygonMode);
    sceGxmSetBackPolygonMode(g_probeContext, polygonMode);

    void* uniformBuffer = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniformBuffer) < 0 || !uniformBuffer)
        return false;

    sceGxmSetUniformDataF(
        uniformBuffer,
        subdividedGouraud
            ? g_gouraudSubdivWvpParam
            : (gouraudPath
                ? g_gouraudPayloadWvpParam
                : (textured ? g_textureWvpParam : g_probeWvpParam)),
        0, 16, drawState.wvp);

    const void* vertexStream =
        subdividedGouraud
            ? static_cast<const void*>(g_vdp1SubdivVertices)
            : (gouraudPath
            ? static_cast<const void*>(g_vdp1GouraudVertices)
            : (textured
                ? static_cast<const void*>(g_vdp1TextureVertices)
                : static_cast<const void*>(g_vdp1Vertices)));

    if (sceGxmSetVertexStream(g_probeContext, 0, vertexStream) < 0)
        return false;

    if (textured) {
        const bool liveVisibility =
            g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
            g_liveTownGouraudPrepValid &&
            g_liveTownGouraudPrepPolygonCount == model.polygonCount &&
            g_liveTownGouraudPrep.size() == model.polygonCount;

        const unsigned int bucketCount =
            static_cast<unsigned int>(g_vdp1GpuTextures.size());
        if (!bucketCount)
            return false;

        static std::vector<unsigned int> counts;
        static std::vector<unsigned int> writes;
        counts.resize(bucketCount);
        writes.resize(bucketCount);

        const auto polygonVisible = [&](unsigned int p) {
            return !liveVisibility || g_liveTownGouraudPrep[p].visible;
        };

        const auto submitRange = [&](std::size_t first,
                                     std::size_t count,
                                     bool orderedShadow) -> bool {
            if (!count)
                return true;

            const std::size_t end =
                std::min(first + count, model.polygonCount);
            if (first >= end)
                return true;

            std::fill(counts.begin(), counts.end(), 0u);
            std::fill(writes.begin(), writes.end(), 0u);

            for (std::size_t p = first; p < end; ++p) {
                if (!polygonVisible(static_cast<unsigned int>(p)))
                    continue;
                const std::uint16_t textureIndex =
                    model.polygonTextureIndices[p];
                if (textureIndex < bucketCount)
                    counts[textureIndex] += 6u;
            }

            // Each ordered phase gets an immutable slice of the mapped
            // index buffer. GXM consumes draws asynchronously, so reusing
            // offset zero for world -> shadow -> Edge would let later CPU
            // writes corrupt indices still referenced by earlier draws.
            const unsigned int phaseBase =
                static_cast<unsigned int>(first * 6u);
            const unsigned int phaseCapacity =
                static_cast<unsigned int>((end - first) * 6u);
            unsigned int total = 0u;
            if (g_vdp1TextureBatches.size() < bucketCount)
                g_vdp1TextureBatches.resize(bucketCount);
            for (unsigned int t = 0; t < bucketCount; ++t) {
                g_vdp1TextureBatches[t].firstIndex =
                    phaseBase + total;
                g_vdp1TextureBatches[t].indexCount = counts[t];
                writes[t] = phaseBase + total;
                total += counts[t];
            }
            if (total > phaseCapacity ||
                phaseBase + total > model.vertexCount)
                return false;

            for (std::size_t p = first; p < end; ++p) {
                if (!polygonVisible(static_cast<unsigned int>(p)))
                    continue;
                const std::uint16_t textureIndex =
                    model.polygonTextureIndices[p];
                if (textureIndex >= bucketCount)
                    continue;
                unsigned int& write = writes[textureIndex];
                for (unsigned int k = 0; k < 6u; ++k)
                    g_vdp1TextureIndices[write++] =
                        static_cast<std::uint16_t>(p * 6u + k);
            }

            for (unsigned int t = 0; t < bucketCount; ++t) {
                const TextureBatch& batch = g_vdp1TextureBatches[t];
                if (!batch.indexCount)
                    continue;

                const bool mesh =
                    orderedShadow || g_vdp1GpuTextures[t].mesh;
                if (orderedShadow) {
                    static unsigned int orderedShadowTraceBudget = 8u;
                    if (orderedShadowTraceBudget != 0u) {
                        logging::writef(
                            "[PresentationTrace][MeshDraw] tex=%u indices=%u "
                            "mesh=%u\n",
                            t,
                            batch.indexCount,
                            mesh ? 1u : 0u);
                        --orderedShadowTraceBudget;
                    }
                    // Saturn VDP1 mesh mode is ordered overdraw, not a z-buffered
                    // material. Preserve this Azel-authored command phase
                    // explicitly rather than allowing texture batching/depth
                    // testing to reorder or reject it.
                    sceGxmSetFrontPolygonMode(
                        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
                    sceGxmSetBackPolygonMode(
                        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
                    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
                    sceGxmSetFrontDepthFunc(
                        g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
                    sceGxmSetBackDepthFunc(
                        g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
                    sceGxmSetFrontDepthWriteEnable(
                        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
                    sceGxmSetBackDepthWriteEnable(
                        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
                }

                sceGxmSetFragmentProgram(
                    g_probeContext,
                    mesh ? g_meshTextureFragmentProgram
                         : g_textureFragmentProgram);
                sceGxmSetFragmentTexture(
                    g_probeContext, 0, &g_vdp1GpuTextures[t].texture);
                sceGxmDraw(
                    g_probeContext,
                    SCE_GXM_PRIMITIVE_TRIANGLES,
                    SCE_GXM_INDEX_FORMAT_U16,
                    g_vdp1TextureIndices + batch.firstIndex,
                    batch.indexCount);

                if (orderedShadow) {
                    sceGxmSetCullMode(g_probeContext, cullMode);
                    sceGxmSetFrontDepthFunc(g_probeContext, depthFunc);
                    sceGxmSetBackDepthFunc(g_probeContext, depthFunc);
                    sceGxmSetFrontDepthWriteEnable(
                        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
                    sceGxmSetBackDepthWriteEnable(
                        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
                    sceGxmSetFrontPolygonMode(g_probeContext, polygonMode);
                    sceGxmSetBackPolygonMode(g_probeContext, polygonMode);
                }
            }
            return true;
        };

        if (g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
            g_liveTownEdgePolygonCount != 0u) {
            const std::size_t worldEnd =
                g_liveTownShadowPolygonCount != 0u
                    ? g_liveTownShadowFirstPolygon
                    : g_liveTownEdgeFirstPolygon;

            // 1. Town/world and task-owned objects.
            if (!submitRange(0u, worldEnd, false))
                return false;

            // 2. Edge's ordered VDP1 mesh shadow.
            if (g_liveTownShadowPolygonCount != 0u &&
                !submitRange(
                    g_liveTownShadowFirstPolygon,
                    g_liveTownShadowPolygonCount,
                    true))
                return false;

            // 3. Edge actor. Normal depth testing now places Edge over its
            // shadow while the already-rendered town remains underneath.
            if (!submitRange(
                    g_liveTownEdgeFirstPolygon,
                    g_liveTownEdgePolygonCount,
                    false))
                return false;

            const std::size_t tailFirst =
                g_liveTownEdgeFirstPolygon +
                g_liveTownEdgePolygonCount;
            if (tailFirst < model.polygonCount &&
                !submitRange(
                    tailFirst,
                    model.polygonCount - tailFirst,
                    false))
                return false;
            return true;
        }

        if (g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
            !g_liveTownMeshRanges.empty()) {
            std::size_t cursor = 0u;
            for (const auto& meshRange : g_liveTownMeshRanges) {
                if (meshRange.first > cursor &&
                    !submitRange(
                        cursor,
                        meshRange.first - cursor,
                        false))
                    return false;

                if (!submitRange(
                        meshRange.first,
                        meshRange.count,
                        true))
                    return false;

                cursor = meshRange.first + meshRange.count;
            }
            if (cursor < model.polygonCount &&
                !submitRange(
                    cursor,
                    model.polygonCount - cursor,
                    false))
                return false;
            return true;
        }

        return submitRange(0u, model.polygonCount, false);
    }

    if (subdividedGouraud) {
        const bool useLiveTownPrep =
            g_liveTownGouraudPrepValid &&
            model.vertices == g_liveTownGouraudPrepVertices &&
            model.polygonCount == g_liveTownGouraudPrepPolygonCount &&
            g_liveTownGouraudPrep.size() == model.polygonCount;
        if (!useLiveTownPrep)
            return false;

        g_profileGouraudProjectUs = 0u;
        g_profileGouraudPayloadUs = 0u;
        g_profileGouraudBucketUs = 0u;
        g_profileGouraudIndexUs = 0u;
        g_profileGouraudDrawUs = 0u;

        static const unsigned int cornerVertex[4] = {0u, 1u, 2u, 5u};

        const unsigned int bucketCount =
            subdividedTexturedLit
                ? static_cast<unsigned int>(g_vdp1GpuTextures.size())
                : 1u;
        if (!bucketCount)
            return false;

        static std::vector<unsigned int> batchCounts;
        static std::vector<unsigned int> batchWrite;
        static std::vector<std::uint8_t> visibleQuads;
        batchCounts.assign(bucketCount, 0u);
        batchWrite.assign(bucketCount, 0u);
        visibleQuads.assign(model.polygonCount, 0u);

        const std::uint64_t tPayload = sceKernelGetProcessTimeWide();
        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            if (!g_liveTownGouraudPrep[p].visible)
                continue;
            if (subdividedGouraudGray &&
                (model.polygons[p].cmdPmod & 0x0100u))
                continue;

            const std::uint16_t textureIndex =
                subdividedTexturedLit ? model.polygonTextureIndices[p] : 0u;
            if (textureIndex >= bucketCount)
                continue;

            const auto bilerp = [](
                float a, float b, float c, float d,
                float u, float v) {
                const float top = a + (b - a) * u;
                const float bottom = d + (c - d) * u;
                return top + (bottom - top) * v;
            };

            const auto& a =
                model.vertices[p * 6u + cornerVertex[0]];
            const auto& b =
                model.vertices[p * 6u + cornerVertex[1]];
            const auto& c =
                model.vertices[p * 6u + cornerVertex[2]];
            const auto& d =
                model.vertices[p * 6u + cornerVertex[3]];
            const auto& shade = model.gouraud555[p];

            const bool updatePosition =
                g_liveTownStaticRebuilt ||
                p >= static_cast<unsigned int>(
                    g_liveTownStaticPolygonCount);
            const unsigned int baseVertex = p * 9u;
            for (unsigned gy = 0; gy < 3u; ++gy) {
                const float v = static_cast<float>(gy) * 0.5f;
                for (unsigned gx = 0; gx < 3u; ++gx) {
                    const float u =
                        static_cast<float>(gx) * 0.5f;
                    auto& dst =
                        g_vdp1SubdivVertices[
                            baseVertex + gy * 3u + gx];

                    if (updatePosition) {
                        dst.x = bilerp(a.x,b.x,c.x,d.x,u,v);
                        dst.y = bilerp(a.y,b.y,c.y,d.y,u,v);
                        dst.z = bilerp(a.z,b.z,c.z,d.z,u,v);
                    }

                    dst.shadeR = bilerp(
                        shade.corner[0][0], shade.corner[1][0],
                        shade.corner[2][0], shade.corner[3][0],
                        u, v);
                    dst.shadeG = bilerp(
                        shade.corner[0][1], shade.corner[1][1],
                        shade.corner[2][1], shade.corner[3][1],
                        u, v);
                    dst.shadeB = bilerp(
                        shade.corner[0][2], shade.corner[1][2],
                        shade.corner[2][2], shade.corner[3][2],
                        u, v);
                }
            }

            visibleQuads[p] = 1u;
            batchCounts[textureIndex] += 24u;
        }
        g_profileGouraudPayloadUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - tPayload);

        const std::uint64_t tBucket = sceKernelGetProcessTimeWide();
        if (g_vdp1TextureBatches.size() < bucketCount)
            g_vdp1TextureBatches.resize(bucketCount);
        g_profileGouraudBucketUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - tBucket);

        const auto submitSubdivRange = [&](std::size_t first,
                                           std::size_t count,
                                           bool orderedShadow) -> bool {
            if (!count)
                return true;

            const std::size_t end =
                std::min(first + count, model.polygonCount);
            if (first >= end)
                return true;

            std::fill(batchCounts.begin(), batchCounts.end(), 0u);
            std::fill(batchWrite.begin(), batchWrite.end(), 0u);

            for (std::size_t p = first; p < end; ++p) {
                if (!visibleQuads[p])
                    continue;
                const unsigned int bucket =
                    subdividedTexturedLit
                        ? model.polygonTextureIndices[p] : 0u;
                if (bucket < bucketCount)
                    batchCounts[bucket] += 24u;
            }

            // As above, preserve every phase's submitted indices until
            // sceGxmEndScene/Finish. Full mode uses 24 generated indices per
            // original Saturn quad, so the source polygon range maps directly
            // to a non-overlapping buffer slice.
            const unsigned int phaseBase =
                static_cast<unsigned int>(first * 24u);
            const unsigned int phaseCapacity =
                static_cast<unsigned int>((end - first) * 24u);
            unsigned int totalVisibleIndices = 0u;
            for (unsigned int t = 0; t < bucketCount; ++t) {
                g_vdp1TextureBatches[t].firstIndex =
                    phaseBase + totalVisibleIndices;
                g_vdp1TextureBatches[t].indexCount = batchCounts[t];
                batchWrite[t] =
                    phaseBase + totalVisibleIndices;
                totalVisibleIndices += batchCounts[t];
            }
            if (totalVisibleIndices > phaseCapacity ||
                phaseBase + totalVisibleIndices >
                    g_vdp1SubdivIndexCapacity)
                return false;

            for (std::size_t p = first; p < end; ++p) {
                if (!visibleQuads[p])
                    continue;
                const unsigned int bucket =
                    subdividedTexturedLit
                        ? model.polygonTextureIndices[p] : 0u;
                if (bucket >= bucketCount)
                    continue;
                unsigned int& write = batchWrite[bucket];
                const unsigned int quadIndexBase =
                    static_cast<unsigned int>(p) * 24u;
                std::memcpy(
                    g_vdp1SubdivIndices + write,
                    g_vdp1SubdivQuadIndices.data() + quadIndexBase,
                    24u * sizeof(std::uint16_t));
                write += 24u;
            }

            for (unsigned int t = 0; t < bucketCount; ++t) {
                const TextureBatch& batch = g_vdp1TextureBatches[t];
                if (!batch.indexCount)
                    continue;

                bool mesh = false;
                if (subdividedTexturedLit) {
                    mesh = orderedShadow || g_vdp1GpuTextures[t].mesh;
                    if (orderedShadow) {
                        sceGxmSetFrontPolygonMode(
                            g_probeContext,
                            SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
                        sceGxmSetBackPolygonMode(
                            g_probeContext,
                            SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
                        sceGxmSetCullMode(
                            g_probeContext, SCE_GXM_CULL_NONE);
                        sceGxmSetFrontDepthFunc(
                            g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
                        sceGxmSetBackDepthFunc(
                            g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
                        sceGxmSetFrontDepthWriteEnable(
                            g_probeContext,
                            SCE_GXM_DEPTH_WRITE_DISABLED);
                        sceGxmSetBackDepthWriteEnable(
                            g_probeContext,
                            SCE_GXM_DEPTH_WRITE_DISABLED);
                    }
                    sceGxmSetFragmentProgram(
                        g_probeContext,
                        mesh ? g_meshSubdivFragmentProgram
                             : g_texturedGouraudSubdivFragmentProgram);
                    sceGxmSetFragmentTexture(
                        g_probeContext, 0,
                        &g_vdp1GpuTextures[t].texture);
                }

                sceGxmDraw(
                    g_probeContext,
                    SCE_GXM_PRIMITIVE_TRIANGLES,
                    SCE_GXM_INDEX_FORMAT_U16,
                    g_vdp1SubdivIndices + batch.firstIndex,
                    batch.indexCount);

                if (orderedShadow) {
                    sceGxmSetCullMode(g_probeContext, cullMode);
                    sceGxmSetFrontDepthFunc(g_probeContext, depthFunc);
                    sceGxmSetBackDepthFunc(g_probeContext, depthFunc);
                    sceGxmSetFrontDepthWriteEnable(
                        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
                    sceGxmSetBackDepthWriteEnable(
                        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
                    sceGxmSetFrontPolygonMode(g_probeContext, polygonMode);
                    sceGxmSetBackPolygonMode(g_probeContext, polygonMode);
                }
            }
            return true;
        };

        const std::uint64_t tIndex = sceKernelGetProcessTimeWide();
        bool phaseResult = true;
        if (subdividedTexturedLit &&
            g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
            g_liveTownEdgePolygonCount != 0u) {
            const std::size_t worldEnd =
                g_liveTownShadowPolygonCount != 0u
                    ? g_liveTownShadowFirstPolygon
                    : g_liveTownEdgeFirstPolygon;

            phaseResult =
                submitSubdivRange(0u, worldEnd, false) &&
                (g_liveTownShadowPolygonCount == 0u ||
                 submitSubdivRange(
                    g_liveTownShadowFirstPolygon,
                    g_liveTownShadowPolygonCount,
                    true)) &&
                submitSubdivRange(
                    g_liveTownEdgeFirstPolygon,
                    g_liveTownEdgePolygonCount,
                    false);

            const std::size_t tailFirst =
                g_liveTownEdgeFirstPolygon +
                g_liveTownEdgePolygonCount;
            if (phaseResult && tailFirst < model.polygonCount)
                phaseResult = submitSubdivRange(
                    tailFirst,
                    model.polygonCount - tailFirst,
                    false);
        } else if (
            subdividedTexturedLit &&
            g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
            !g_liveTownMeshRanges.empty()) {
            std::size_t cursor = 0u;
            for (const auto& meshRange : g_liveTownMeshRanges) {
                if (phaseResult &&
                    meshRange.first > cursor) {
                    phaseResult = submitSubdivRange(
                        cursor,
                        meshRange.first - cursor,
                        false);
                }
                if (phaseResult) {
                    phaseResult = submitSubdivRange(
                        meshRange.first,
                        meshRange.count,
                        true);
                }
                cursor = meshRange.first + meshRange.count;
            }
            if (phaseResult && cursor < model.polygonCount) {
                phaseResult = submitSubdivRange(
                    cursor,
                    model.polygonCount - cursor,
                    false);
            }
        } else {
            phaseResult =
                submitSubdivRange(0u, model.polygonCount, false);
        }
        g_profileGouraudIndexUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - tIndex);
        g_profileGouraudDrawUs = g_profileGouraudIndexUs;
        return phaseResult;
    }

    if (gouraudPath) {
        const bool useLiveTownPrep =
            g_liveTownGouraudPrepValid &&
            g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
            model.vertices == g_liveTownGouraudPrepVertices &&
            model.polygonCount == g_liveTownGouraudPrepPolygonCount &&
            g_liveTownGouraudPrep.size() == model.polygonCount;
        g_profileGouraudProjectUs = 0u;
        g_profileGouraudPayloadUs = 0u;
        g_profileGouraudBucketUs = 0u;
        g_profileGouraudIndexUs = 0u;
        g_profileGouraudDrawUs = 0u;
        if (!useLiveTownPrep) {
            g_profileGouraudVisibleQuads = 0u;
            g_profileGouraudTotalQuads =
                static_cast<unsigned int>(model.polygonCount);
        }

        // Preserve original Saturn quad identity and attach the four recovered
        // Gouraud corner values to every generated triangle vertex. The final
        // shaders interpolate those values with the accepted VDP1-style
        // projected scanline method.
        static const unsigned int cornerVertex[4] = {0, 1, 2, 5};
        ViewerMat4 wvp{};
        std::memcpy(wvp.m, drawState.wvp, sizeof(wvp.m));

        const unsigned int textureBucketCount =
            texturedLit
                ? static_cast<unsigned int>(g_vdp1GpuTextures.size())
                : 1u;
        if (textureBucketCount == 0u)
            return false;

        static std::vector<unsigned int> batchCounts;
        static std::vector<unsigned int> batchWrite;
        static std::vector<std::uint8_t> visibleQuads;
        batchCounts.assign(textureBucketCount, 0u);
        batchWrite.assign(textureBucketCount, 0u);
        visibleQuads.assign(model.polygonCount, 0u);

        const std::uint64_t tProjectPayload = sceKernelGetProcessTimeWide();
        std::uint64_t projectAccumUs = 0u;
        std::uint64_t payloadAccumUs = 0u;
        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            const std::uint16_t textureIndex =
                texturedLit ? model.polygonTextureIndices[p] : 0u;
            if (textureIndex >= textureBucketCount)
                continue;

            ViewerScreenPoint screen[4];
            if (useLiveTownPrep) {
                const auto& prep = g_liveTownGouraudPrep[p];
                if (!prep.visible)
                    continue;
                std::memcpy(screen, prep.screen, sizeof(screen));
            } else {
                const std::uint64_t tProject = sceKernelGetProcessTimeWide();
                bool visible = true;
                std::uint8_t sharedOutcode = 0x3Fu;

                // Preserve the original conservative fallback for diagnostic
                // and non-live Gouraud sources.
                for (unsigned int corner = 0; corner < 4; ++corner) {
                    const auto& v =
                        model.vertices[p * 6u + cornerVertex[corner]];
                    const float clipX =
                        v.x * wvp.m[0] + v.y * wvp.m[4] +
                        v.z * wvp.m[8] + wvp.m[12];
                    const float clipY =
                        v.x * wvp.m[1] + v.y * wvp.m[5] +
                        v.z * wvp.m[9] + wvp.m[13];
                    const float clipZ =
                        v.x * wvp.m[2] + v.y * wvp.m[6] +
                        v.z * wvp.m[10] + wvp.m[14];
                    const float clipW =
                        v.x * wvp.m[3] + v.y * wvp.m[7] +
                        v.z * wvp.m[11] + wvp.m[15];

                    std::uint8_t outcode = 0u;
                    if (clipX < -clipW) outcode |= 1u << 0;
                    if (clipX >  clipW) outcode |= 1u << 1;
                    if (clipY < -clipW) outcode |= 1u << 2;
                    if (clipY >  clipW) outcode |= 1u << 3;
                    if (clipZ <  0.0f)  outcode |= 1u << 4;
                    if (clipZ >  clipW) outcode |= 1u << 5;
                    sharedOutcode &= outcode;

                    if (clipW <= 0.00001f) {
                        visible = false;
                        continue;
                    }

                    const float invW = 1.0f / clipW;
                    screen[corner].x =
                        (clipX * invW * 0.5f + 0.5f) *
                        static_cast<float>(viewerRenderWidth());
                    screen[corner].y =
                        (0.5f - clipY * invW * 0.5f) *
                        static_cast<float>(viewerRenderHeight());
                    screen[corner].valid = true;
                }
                projectAccumUs += sceKernelGetProcessTimeWide() - tProject;
                if (!visible || sharedOutcode != 0u)
                    continue;
                ++g_profileGouraudVisibleQuads;
            }

            visibleQuads[p] = 1u;

            const std::uint64_t tPayload = sceKernelGetProcessTimeWide();
            const auto& gouraud = model.gouraud555[p];
            const float payload[5][4] = {
                {
                    screen[0].x, screen[0].y,
                    screen[1].x, screen[1].y
                },
                {
                    screen[2].x, screen[2].y,
                    screen[3].x, screen[3].y
                },
                {
                    gouraud.corner[0][0],
                    gouraud.corner[1][0],
                    gouraud.corner[2][0],
                    gouraud.corner[3][0]
                },
                {
                    gouraud.corner[0][1],
                    gouraud.corner[1][1],
                    gouraud.corner[2][1],
                    gouraud.corner[3][1]
                },
                {
                    gouraud.corner[0][2],
                    gouraud.corner[1][2],
                    gouraud.corner[2][2],
                    gouraud.corner[3][2]
                }
            };

            for (unsigned int k = 0; k < 6u; ++k) {
                auto& v = g_vdp1GouraudVertices[p * 6u + k];
                std::memcpy(v.quadScreen01, payload[0], sizeof(payload[0]));
                std::memcpy(v.quadScreen23, payload[1], sizeof(payload[1]));
                std::memcpy(v.gouraudR, payload[2], sizeof(payload[2]));
                std::memcpy(v.gouraudG, payload[3], sizeof(payload[3]));
                std::memcpy(v.gouraudB, payload[4], sizeof(payload[4]));
            }

            batchCounts[textureIndex] += 6u;
            payloadAccumUs += sceKernelGetProcessTimeWide() - tPayload;
        }
        (void)tProjectPayload;
        g_profileGouraudProjectUs = static_cast<unsigned int>(projectAccumUs);
        g_profileGouraudPayloadUs = static_cast<unsigned int>(payloadAccumUs);

        const std::uint64_t tBucket = sceKernelGetProcessTimeWide();
        unsigned int totalVisibleIndices = 0u;
        if (g_vdp1TextureBatches.size() < textureBucketCount)
            g_vdp1TextureBatches.resize(textureBucketCount);

        for (unsigned int t = 0; t < textureBucketCount; ++t) {
            g_vdp1TextureBatches[t].firstIndex = totalVisibleIndices;
            g_vdp1TextureBatches[t].indexCount = batchCounts[t];
            batchWrite[t] = totalVisibleIndices;
            totalVisibleIndices += batchCounts[t];
        }

        if (totalVisibleIndices >
            static_cast<unsigned int>(model.vertexCount))
            return false;
        g_profileGouraudBucketUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - tBucket);

        const std::uint64_t tIndex = sceKernelGetProcessTimeWide();
        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            if (!visibleQuads[p])
                continue;

            const std::uint16_t textureIndex =
                texturedLit ? model.polygonTextureIndices[p] : 0u;
            if (textureIndex >= textureBucketCount ||
                batchCounts[textureIndex] == 0u)
                continue;

            unsigned int& write = batchWrite[textureIndex];
            for (unsigned int k = 0; k < 6u; ++k) {
                g_vdp1TextureIndices[write++] =
                    static_cast<std::uint16_t>(p * 6u + k);
            }
        }

        g_profileGouraudIndexUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - tIndex);

        const std::uint64_t tDraw = sceKernelGetProcessTimeWide();
        unsigned int submittedBatches = 0u;
        for (unsigned int t = 0; t < textureBucketCount; ++t) {
            const TextureBatch& batch = g_vdp1TextureBatches[t];
            if (!batch.indexCount)
                continue;

            if (texturedLit) {
                sceGxmSetFragmentProgram(
                    g_probeContext, g_texturedLitHalfFragmentProgram);
                sceGxmSetFragmentTexture(
                    g_probeContext, 0, &g_vdp1GpuTextures[t].texture);
            }

            sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_vdp1TextureIndices + batch.firstIndex,
                batch.indexCount);
            ++submittedBatches;
        }

        g_profileGouraudDrawUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - tDraw);
        return submittedBatches != 0u ||
               totalVisibleIndices == 0u;
    }

    if (wireframe &&
        g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
        g_liveTownGouraudPrepValid &&
        g_liveTownGouraudPrepPolygonCount == model.polygonCount &&
        g_liveTownGouraudPrep.size() == model.polygonCount) {
        // The CPU model expands each original Saturn quad to:
        //     A,B,C / A,C,D
        // so triangle wireframe exposes an artificial A-C diagonal. Wires is
        // intended to visualize the source VDP1 quads, not the GXM
        // triangulation. Submit the four original perimeter edges explicitly.
        static const unsigned int cornerVertex[4] = {0u, 1u, 2u, 5u};
        static const unsigned int edgeCorners[8] = {
            0u,1u, 1u,2u, 2u,3u, 3u,0u
        };

        unsigned int write = 0u;
        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            if (!g_liveTownGouraudPrep[p].visible)
                continue;

            const unsigned int base = p * 6u;
            for (unsigned int k = 0; k < 8u; ++k) {
                g_vdp1Indices[write++] =
                    static_cast<std::uint16_t>(
                        base + cornerVertex[edgeCorners[k]]);
            }
        }

        if (write) {
            sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_LINES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_vdp1Indices,
                write);
        }

        // Visualize the two interior boundaries of Full mode's 3x3 vertex
        // grid (four sub-quads). These are diagnostic-only and deliberately
        // half as bright as the interpolated source-quad outline.
        if (g_vdp1SubdivWireVertices &&
            g_vdp1SubdivWireIndices &&
            g_vdp1SubdivWireVertexCapacity >=
                model.polygonCount * 4u) {
            unsigned int subdivWrite = 0u;

            auto midpoint = [](
                const azel::DebugColorVertex& a,
                const azel::DebugColorVertex& b) {
                azel::DebugColorVertex v{};
                v.x = (a.x + b.x) * 0.5f;
                v.y = (a.y + b.y) * 0.5f;
                v.z = (a.z + b.z) * 0.5f;

                // Midpoint color would normally be (a+b)/2. Divide that by
                // two again so the subdivision line is exactly 50% brightness.
                v.r = static_cast<std::uint8_t>(
                    (static_cast<unsigned int>(a.r) + b.r) / 4u);
                v.g = static_cast<std::uint8_t>(
                    (static_cast<unsigned int>(a.g) + b.g) / 4u);
                v.b = static_cast<std::uint8_t>(
                    (static_cast<unsigned int>(a.b) + b.b) / 4u);
                v.a = 255u;
                return v;
            };

            for (unsigned int p = 0;
                 p < static_cast<unsigned int>(model.polygonCount); ++p) {
                if (!g_liveTownGouraudPrep[p].visible)
                    continue;

                const unsigned int base = p * 6u;
                const auto& a = model.vertices[base + cornerVertex[0]];
                const auto& b = model.vertices[base + cornerVertex[1]];
                const auto& c = model.vertices[base + cornerVertex[2]];
                const auto& d = model.vertices[base + cornerVertex[3]];

                // Vertical center boundary: midpoint(AB) -> midpoint(DC).
                g_vdp1SubdivWireVertices[subdivWrite++] =
                    midpoint(a, b);
                g_vdp1SubdivWireVertices[subdivWrite++] =
                    midpoint(d, c);

                // Horizontal center boundary: midpoint(AD) -> midpoint(BC).
                g_vdp1SubdivWireVertices[subdivWrite++] =
                    midpoint(a, d);
                g_vdp1SubdivWireVertices[subdivWrite++] =
                    midpoint(b, c);
            }

            if (subdivWrite) {
                sceGxmSetVertexStream(
                    g_probeContext, 0, g_vdp1SubdivWireVertices);
                sceGxmDraw(
                    g_probeContext,
                    SCE_GXM_PRIMITIVE_LINES,
                    SCE_GXM_INDEX_FORMAT_U16,
                    g_vdp1SubdivWireIndices,
                    subdivWrite);

                // Restore the normal stream even though this branch returns;
                // keeping submission state local makes later refactors safe.
                sceGxmSetVertexStream(
                    g_probeContext, 0, g_vdp1Vertices);
            }
        }
        return true;
    }

    // Auth Flat is the cleanest benchmark for backend submission cost. Keep
    // Azel's submitted model set intact and compact only the final index list:
    // reject a quad iff all four original Saturn corners lie outside one same
    // clip plane. Intersecting/partially clipped quads are always preserved.
    if (drawState.mode == Vdp1RenderMode::PolygonColor &&
        g_viewMode == 9 &&
        g_residentVdp1Model == ResidentVdp1Model::LiveTown &&
        model.vertexCount == model.polygonCount * 6u) {
        static const unsigned int cornerVertex[4] = {0u, 1u, 2u, 5u};
        ViewerMat4 wvp{};
        std::memcpy(wvp.m, drawState.wvp, sizeof(wvp.m));

        g_authFlatTotalQuads =
            static_cast<unsigned int>(model.polygonCount);
        g_authFlatVisibleQuads = 0u;
        g_authFlatDrawCalls = 0u;

        // Keep both index lists in the GXM-mapped index buffer. Passing a
        // std::vector's host pointer directly to sceGxmDraw is invalid on Vita
        // and can crash the GPU driver.
        unsigned int ordinaryWrite = 0u;
        unsigned int meshWrite =
            static_cast<unsigned int>(model.vertexCount);

        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            std::uint8_t sharedOutcode = 0x3Fu;
            for (unsigned int corner = 0; corner < 4u; ++corner) {
                sharedOutcode &= viewerClipOutcode(
                    wvp,
                    model.vertices[p * 6u + cornerVertex[corner]]);
            }
            if (sharedOutcode != 0u)
                continue;

            ++g_authFlatVisibleQuads;
            const bool mesh = (model.polygons[p].cmdPmod & 0x0100u) != 0u;
            if (mesh) {
                meshWrite -= 6u;
                for (unsigned int k = 0; k < 6u; ++k)
                    g_vdp1Indices[meshWrite + k] =
                        static_cast<std::uint16_t>(p * 6u + k);
            } else {
                for (unsigned int k = 0; k < 6u; ++k)
                    g_vdp1Indices[ordinaryWrite++] =
                        static_cast<std::uint16_t>(p * 6u + k);
            }
        }

        if (ordinaryWrite != 0u) {
            sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_vdp1Indices,
                ordinaryWrite);
            ++g_authFlatDrawCalls;
        }

        const unsigned int meshCount =
            static_cast<unsigned int>(model.vertexCount) - meshWrite;
        if (meshCount != 0u) {
            sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
            sceGxmSetFrontDepthFunc(
                g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
            sceGxmSetBackDepthFunc(
                g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
            sceGxmSetFrontDepthWriteEnable(
                g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
            sceGxmSetBackDepthWriteEnable(
                g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
            sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_vdp1Indices + meshWrite,
                meshCount);
            sceGxmSetCullMode(g_probeContext, cullMode);
            sceGxmSetFrontDepthFunc(g_probeContext, depthFunc);
            sceGxmSetBackDepthFunc(g_probeContext, depthFunc);
            sceGxmSetFrontDepthWriteEnable(
                g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
            sceGxmSetBackDepthWriteEnable(
                g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
            ++g_authFlatDrawCalls;
        }
        return true;
    }

    sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_vdp1Indices,
        static_cast<unsigned int>(model.vertexCount));
    return true;
}

static float updateTownFadeAlpha()
{
    if (g_townFadeSerial != g_townFadeAppliedSerial) {
        g_townFadeAppliedSerial = g_townFadeSerial;
        g_townFadeElapsed = 0u;
    }

    const unsigned int duration = std::max(1u, g_townFadeFrames);
    if (g_townFadeElapsed < duration) {
        const float t =
            static_cast<float>(g_townFadeElapsed + 1u) /
            static_cast<float>(duration);
        g_townFadeBlack = g_townFadeIn
            ? std::max(0.0f, 1.0f - t)
            : std::min(1.0f, t);
        ++g_townFadeElapsed;
    } else {
        g_townFadeBlack = g_townFadeIn ? 0.0f : 1.0f;
    }
    return g_townFadeBlack;
}

static void drawFadeOverlay(
    float alpha,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue)
{
    if (alpha <= 0.0f || !g_fadeVertices || !g_fadeIndices ||
        !g_fadeFragmentProgram)
        return;

    const std::uint8_t a = static_cast<std::uint8_t>(
        std::clamp<int>(
            static_cast<int>(std::lround(alpha * 255.0f)), 0, 255));
    for (unsigned i = 0; i < 6u; ++i) {
        g_fadeVertices[i].r = red;
        g_fadeVertices[i].g = green;
        g_fadeVertices[i].b = blue;
        g_fadeVertices[i].a = a;
    }

    sceGxmSetVertexProgram(g_probeContext, g_probeVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext, g_fadeFragmentProgram);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetFrontDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);

    void* uniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniforms) >= 0 && uniforms) {
        const ViewerMat4 identity = viewerIdentity();
        sceGxmSetUniformDataF(
            uniforms, g_probeWvpParam, 0, 16, identity.m);
        sceGxmSetVertexStream(g_probeContext, 0, g_fadeVertices);
        sceGxmDraw(
            g_probeContext,
            SCE_GXM_PRIMITIVE_TRIANGLES,
            SCE_GXM_INDEX_FORMAT_U16,
            g_fadeIndices,
            6u);
    }
}

static void drawColorOffsetPass(
    SceGxmFragmentProgram* program,
    int red, int green, int blue)
{
    if (!program || !g_fadeVertices || !g_fadeIndices)
        return;
    if (red <= 0 && green <= 0 && blue <= 0)
        return;

    const std::uint8_t r = static_cast<std::uint8_t>(
        std::clamp(red, 0, 255));
    const std::uint8_t g = static_cast<std::uint8_t>(
        std::clamp(green, 0, 255));
    const std::uint8_t b = static_cast<std::uint8_t>(
        std::clamp(blue, 0, 255));

    for (unsigned i = 0; i < 6u; ++i) {
        g_fadeVertices[i].r = r;
        g_fadeVertices[i].g = g;
        g_fadeVertices[i].b = b;
        g_fadeVertices[i].a = 255u;
    }

    sceGxmSetVertexProgram(g_probeContext, g_probeVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext, program);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetFrontDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);

    void* uniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniforms) < 0 || !uniforms)
        return;

    const ViewerMat4 identity = viewerIdentity();
    sceGxmSetUniformDataF(
        uniforms, g_probeWvpParam, 0, 16, identity.m);
    sceGxmSetVertexStream(g_probeContext, 0, g_fadeVertices);
    sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_fadeIndices,
        6u);
}

static void drawAzelColorOffsetForLayer(
    unsigned int layerBit,
    bool requireEnable)
{
    // Saturn VDP2 CLOFEN/CLOFSL bit assignment:
    //   bit 6 SPRITE, bit 5 BACK, bit 4 RBG0,
    //   bit 3 NBG3, bit 2 NBG2, bit 1 NBG1, bit 0 NBG0.
    //
    // Some front-end fade paths in the reconstructed Azel runtime clear
    // CLOFEN while their live fade channel still carries the intended
    // fullscreen transition. Those compatibility paths can opt out of the
    // enable test, but real layer composition (D5 RBG0) must honor CLOFEN.
    const unsigned int enable =
        g_azelColorOffsetEnable.load(std::memory_order_relaxed);
    if (requireEnable && (enable & layerBit) == 0u)
        return;

    const unsigned int select =
        g_azelColorOffsetSelect.load(std::memory_order_relaxed);
    const bool useB = (select & layerBit) != 0u;

    const int red = useB
        ? g_azelColorOffsetBRed.load(std::memory_order_relaxed)
        : g_azelColorOffsetARed.load(std::memory_order_relaxed);
    const int green = useB
        ? g_azelColorOffsetBGreen.load(std::memory_order_relaxed)
        : g_azelColorOffsetAGreen.load(std::memory_order_relaxed);
    const int blue = useB
        ? g_azelColorOffsetBBlue.load(std::memory_order_relaxed)
        : g_azelColorOffsetABlue.load(std::memory_order_relaxed);

    // Saturn VDP2 color-offset registers are signed 9-bit values. Azel's
    // host reconstruction stores them in s16, so intermediate fade math can
    // legitimately run outside the hardware range (for example -263). Real
    // VDP2 keeps only the low nine bits on register write; reproduce that
    // wrap before translating the offset to GXM instead of clamping it.
    auto hardwareSigned9 = [](int value) -> int {
        const unsigned int raw =
            static_cast<unsigned int>(value) & 0x1FFu;
        return (raw & 0x100u)
            ? static_cast<int>(raw) - 0x200
            : static_cast<int>(raw);
    };

    // Azel's fade reconstruction expands Saturn's signed 5-bit color
    // endpoints to approximately -128..+135 (see unpackColor(): negative
    // values * 8, positive values * 9). Neptune blends in an 8-bit display
    // domain, so expand that contribution once here. With the CLOFEN/CLOFSL
    // layer mapping now corrected this reaches true black/white without
    // incorrectly washing unrelated layers.
    const int displayRed =
        std::clamp(hardwareSigned9(red) * 2, -255, 255);
    const int displayGreen =
        std::clamp(hardwareSigned9(green) * 2, -255, 255);
    const int displayBlue =
        std::clamp(hardwareSigned9(blue) * 2, -255, 255);

    drawColorOffsetPass(
        g_colorOffsetAddFragmentProgram,
        std::max(displayRed, 0),
        std::max(displayGreen, 0),
        std::max(displayBlue, 0));
    drawColorOffsetPass(
        g_colorOffsetSubtractFragmentProgram,
        std::max(-displayRed, 0),
        std::max(-displayGreen, 0),
        std::max(-displayBlue, 0));
}

static void drawAzelFrontendFadeOffset()
{
    // The native movie/title state machine drives its transition through the
    // same fade channels even after the reconstructed VDP2 setup has cleared
    // CLOFEN. The platform movie surface stands in for Saturn's composed
    // front-end output, so apply the live channel as a fullscreen compatibility
    // pass. CLOFSL is normally zero in this path; use NBG0's select bit so the
    // A/B choice remains deterministic if Azel changes it.
    constexpr unsigned int kNbg0Bit = 0x01u;
    drawAzelColorOffsetForLayer(kNbg0Bit, true);
}


static bool renderCinepakResolvePass()
{
    if (!g_movieUsesCinepakPayload ||
        !g_cinepakResolveColorBuffer ||
        !g_cinepakResolveVertices ||
        !g_cinepakResolveIndices)
        return false;

    const int beginResult = sceGxmBeginScene(
        g_probeContext,
        0,
        g_movieRenderTarget,
        nullptr,
        nullptr,
        nullptr,
        &g_cinepakResolveColorSurface,
        nullptr);
    if (beginResult < 0) {
        logging::writef(
            "[MovieRender] FAIL Cinepak resolve begin=0x%08X\n",
            static_cast<unsigned int>(beginResult));
        return false;
    }

    sceGxmSetVertexProgram(g_probeContext, g_textureVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext, g_cinepakFragmentProgram);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetDefaultRegionClipAndViewport(
        g_probeContext,
        static_cast<int>(g_movieWidth) - 1,
        static_cast<int>(g_movieHeight) - 1);
    sceGxmSetFrontDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetFrontPolygonMode(
        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(
        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);

    bool submitted = false;
    void* vertexUniforms = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &vertexUniforms) >= 0 &&
        vertexUniforms) {
        const float identity[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        sceGxmSetUniformDataF(
            vertexUniforms, g_textureWvpParam, 0, 16, identity);
        // First pass reads the compact point-sampled Cinepak payload.
        sceGxmSetFragmentTexture(g_probeContext, 0, &g_movieTexture);

        if (sceGxmSetVertexStream(
                g_probeContext, 0, g_cinepakResolveVertices) >= 0) {
            void* fragmentUniforms = nullptr;
            if (sceGxmReserveFragmentDefaultUniformBuffer(
                    g_probeContext, &fragmentUniforms) >= 0 &&
                fragmentUniforms) {
                const float movieInfo[4] = {
                    static_cast<float>(g_movieWidth),
                    static_cast<float>(g_movieHeight),
                    static_cast<float>(g_moviePayloadWidth),
                    static_cast<float>(g_moviePayloadHeight),
                };
                sceGxmSetUniformDataF(
                    fragmentUniforms,
                    g_cinepakMovieInfoParam,
                    0, 4, movieInfo);
                submitted = sceGxmDraw(
                    g_probeContext,
                    SCE_GXM_PRIMITIVE_TRIANGLES,
                    SCE_GXM_INDEX_FORMAT_U16,
                    g_cinepakResolveIndices,
                    6) >= 0;
            }
        }
    }

    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    if (!submitted)
        logging::writef("[MovieRender] FAIL Cinepak resolve draw\n");
    return submitted;
}

static bool renderMovieFrame()
{
    const std::uint64_t frameStartUs =
        sceKernelGetProcessTimeWide();
    MovieFrameGuard guard;
    if (!guard)
        return false;

    if (!g_movieFrameVisible || !g_movieTextureData ||
        !g_movieVertices || !g_movieIndices ||
        !g_probeContext || !g_movieRenderTarget ||
        !g_textureVertexProgram || !g_movieTextureFragmentProgram ||
        !g_textureWvpParam)
        return false;
    if (g_movieUsesCinepakPayload &&
        (!g_cinepakFragmentProgram || !g_cinepakMovieInfoParam))
        return false;
    if (g_movieUsesVdp2Title &&
        (!g_vdp2NbgFragmentProgram || !g_vdp2InfoParam))
        return false;

    // Azel switches the title to HRESO=3 with double-density interlace.
    // When that live TVMD mode is active, render and scan out a native
    // 720x408 Vita framebuffer directly. There is no 960x544 intermediate
    // and no Neptune scaling pass; sceDisplay receives the 720x408 frame.
    const bool highResolutionFrontend =
        g_movieUsesVdp2Title &&
        (g_movieVdp2Tvmd & 0x00C7u) == 0x00C3u;
    static int lastFrontendDisplayMode = -1;
    const int frontendDisplayMode = highResolutionFrontend ? 1 : 0;
    if (g_movieUsesVdp2Title &&
        frontendDisplayMode != lastFrontendDisplayMode) {
        logging::writef(
            "[VDP2Display] TVMD=%04X framebuffer=%s\n",
            g_movieVdp2Tvmd & 0xFFFFu,
            highResolutionFrontend ? "720x408" : "480x272");
        lastFrontendDisplayMode = frontendDisplayMode;
    }
    const int pitch = highResolutionFrontend ? 1024 : 512;
    const int movieOutputWidth =
        highResolutionFrontend ? 720 : (kWidth / 2);
    const int movieOutputHeight =
        highResolutionFrontend ? 408 : (kHeight / 2);
    std::uint32_t* const colorBuffer =
        g_gxmDrawBuffer == 0 ? g_probeColorBuffer : g_probeColorBuffer2;
    SceGxmColorSurface* const colorSurface =
        g_gxmDrawBuffer == 0
            ? (highResolutionFrontend
                ? &g_frontendHighColorSurface
                : &g_movieColorSurface)
            : (highResolutionFrontend
                ? &g_frontendHighColorSurface2
                : &g_movieColorSurface2);
    SceGxmRenderTarget* const activeRenderTarget =
        highResolutionFrontend
            ? g_frontendHighRenderTarget
            : g_movieRenderTarget;
    SceGxmSyncObject* const syncObject =
        g_gxmDrawBuffer == 0 ? g_probeSync : g_probeSync2;

    if (g_movieUsesCinepakPayload && !renderCinepakResolvePass())
        return true;

    std::memset(
        colorBuffer,
        0,
        static_cast<std::size_t>(pitch) * movieOutputHeight *
            sizeof(std::uint32_t));

    const int beginResult = sceGxmBeginScene(
        g_probeContext,
        0,
        activeRenderTarget,
        nullptr,
        nullptr,
        syncObject,
        colorSurface,
        nullptr);
    if (beginResult < 0) {
        logging::writef(
            "[MovieRender] FAIL sceGxmBeginScene=0x%08X\n",
            static_cast<unsigned int>(beginResult));
        return true;
    }

    sceGxmSetVertexProgram(g_probeContext, g_textureVertexProgram);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetDefaultRegionClipAndViewport(
        g_probeContext, movieOutputWidth - 1, movieOutputHeight - 1);
    sceGxmSetFrontDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetFrontPolygonMode(
        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(
        g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);

    void* uniformBuffer = nullptr;
    bool submitted = false;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniformBuffer) >= 0 && uniformBuffer) {
        const float identity[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        sceGxmSetUniformDataF(
            uniformBuffer, g_textureWvpParam, 0, 16, identity);
        // Cinepak's second pass samples the reconstructed RGBA frame;
        // other movie/front-end paths continue sampling their normal texture.
        sceGxmSetFragmentTexture(
            g_probeContext,
            0,
            g_movieUsesCinepakPayload
                ? &g_cinepakResolveTexture
                : &g_movieTexture);

        const bool streamReady =
            sceGxmSetVertexStream(
                g_probeContext, 0, g_movieVertices) >= 0;

        if (streamReady && g_movieUsesVdp2Title &&
            g_movieVdp2Info[0] < 0.5f && g_titleDecodedValid) {
            sceGxmSetFragmentTexture(
                g_probeContext, 0, &g_titleDecodedTexture);
            sceGxmSetFragmentProgram(
                g_probeContext, g_movieTextureFragmentProgram);
            submitted = sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_movieIndices,
                6) >= 0;
        } else if (streamReady && g_movieUsesVdp2Title) {
            // D5 still consumes live raw VDP2 state for RBG0/NBG0 composition.
            bool rbgSubmitted = true;
            if (g_movieVdp2Info[0] >= 0.5f && g_vdp2Rbg0Available &&
                g_vdp2Rbg0FragmentProgram && g_vdp2Rbg0InfoParam) {
                sceGxmSetFragmentProgram(
                    g_probeContext, g_vdp2Rbg0FragmentProgram);

                const unsigned int rpmd =
                    static_cast<unsigned int>(g_movieRbg0Ctrl[0]) & 3u;
                const unsigned int ktctl =
                    static_cast<unsigned int>(g_movieRbg0Ctrl[1]);
                const unsigned int ktaof =
                    static_cast<unsigned int>(g_movieRbg0Ctrl[2]);
                const unsigned int wctld =
                    static_cast<unsigned int>(g_movieRbg0Ctrl[4]);
                const unsigned int lineWindowMask =
                    static_cast<unsigned int>(g_movieRbg0Ctrl[7]);

                auto submitRbgParameter =
                    [&](const float* planes,
                        const float* transform,
                        const float* coefficient,
                        unsigned int coefficientEnableBit,
                        unsigned int coefficientSizeBit,
                        unsigned int coefficientOffsetShift,
                        const azel::DebugTextureVertex* vertices,
                        const std::uint16_t* indices,
                        unsigned int indexCount) -> bool {
                    void* rbgUniforms = nullptr;
                    if (sceGxmReserveFragmentDefaultUniformBuffer(
                            g_probeContext, &rbgUniforms) < 0 ||
                        !rbgUniforms)
                        return false;

                    for (unsigned int i = 0; i < 4u; ++i) {
                        sceGxmSetUniformDataF(
                            rbgUniforms,
                            g_vdp2Rbg0PlaneParam[i],
                            0, 4,
                            &planes[i * 4u]);
                    }

                    const float coefficientSize =
                        (ktctl & coefficientSizeBit) ? 2.0f : 4.0f;
                    const unsigned int coefficientOffset =
                        (ktaof >> coefficientOffsetShift) & 0x7u;
                    const float rbgInfo[4] = {
                        static_cast<float>(coefficientOffset) *
                            coefficientSize * 65536.0f,
                        (ktctl & coefficientEnableBit) ? 1.0f : 0.0f,
                        coefficientSize,
                        g_movieRbg0Plsz,
                    };

                    sceGxmSetUniformDataF(
                        rbgUniforms,
                        g_vdp2Rbg0Transform0Param,
                        0, 4, &transform[0]);
                    sceGxmSetUniformDataF(
                        rbgUniforms,
                        g_vdp2Rbg0Transform1Param,
                        0, 4, &transform[4]);
                    sceGxmSetUniformDataF(
                        rbgUniforms,
                        g_vdp2Rbg0CoefficientParam,
                        0, 4, coefficient);
                    sceGxmSetUniformDataF(
                        rbgUniforms,
                        g_vdp2Rbg0InfoParam,
                        0, 4, rbgInfo);
                    sceGxmSetUniformDataF(
                        rbgUniforms,
                        g_vdp2Rbg0FormatParam,
                        0, 4, g_movieRbg0Format);

                    if (sceGxmSetVertexStream(
                            g_probeContext, 0, vertices) < 0)
                        return false;

                    return sceGxmDraw(
                        g_probeContext,
                        SCE_GXM_PRIMITIVE_TRIANGLES,
                        SCE_GXM_INDEX_FORMAT_U16,
                        indices,
                        indexCount) >= 0;
                };

                if (rpmd == 1u || rpmd == 3u) {
                    // Parameter B is the base for RPMD=3.
                    rbgSubmitted = submitRbgParameter(
                        g_movieRbg0PlanesB,
                        g_movieRbg0TransformB,
                        g_movieRbg0CoefficientB,
                        0x100u, 0x200u, 8u,
                        g_movieVertices, g_movieIndices, 6u);
                }

                unsigned int parameterAIndexCount = 6u;
                const azel::DebugTextureVertex* parameterAVertices =
                    g_movieVertices;
                const std::uint16_t* parameterAIndices =
                    g_movieIndices;

                if (rbgSubmitted && rpmd == 3u &&
                    g_vdp2WindowVertices && g_vdp2WindowIndices) {
                    unsigned int lineAddress = 0u;
                    bool drawInside = true;
                    int yStart = 0;
                    int yEnd = 223;
                    bool haveSingleLineWindow = false;

                    if ((wctld & 0x8u) != 0u &&
                        (lineWindowMask & 0x2u) != 0u) {
                        lineAddress =
                            static_cast<unsigned int>(g_movieRbg0Ctrl[6]);
                        drawInside = (wctld & 0x4u) != 0u;
                        yStart = static_cast<int>(g_movieRbg0Ctrl[13]);
                        yEnd = static_cast<int>(g_movieRbg0Ctrl[15]);
                        haveSingleLineWindow = true;
                    } else if ((wctld & 0x2u) != 0u &&
                               (lineWindowMask & 0x1u) != 0u) {
                        lineAddress =
                            static_cast<unsigned int>(g_movieRbg0Ctrl[5]);
                        drawInside = (wctld & 0x1u) != 0u;
                        yStart = static_cast<int>(g_movieRbg0Ctrl[9]);
                        yEnd = static_cast<int>(g_movieRbg0Ctrl[11]);
                        haveSingleLineWindow = true;
                    }

                    if (haveSingleLineWindow) {
                        const auto* rawBytes =
                            static_cast<const unsigned char*>(
                                g_movieTextureData);

                        // Hardware bring-up diagnostic: D5 currently uses
                        // RPMD=3 with line window 1. Preserve the last
                        // hardware-good B-base/A-overlay compositor while we
                        // inspect the authored line-window coverage before
                        // changing parameter ownership semantics.
                        static bool loggedD5RpWindow = false;
                        if (!loggedD5RpWindow) {
                            logging::writef(
                                "[NeptuneVDP2] RPMD3 window WCTLD=%04X "
                                "mask=%u addr=%05X y=%d..%d inside=%u\n",
                                wctld, lineWindowMask, lineAddress,
                                yStart, yEnd, drawInside ? 1u : 0u);
                            const int sampleY[] = {
                                0, 32, 64, 96, 112, 128, 160, 192, 223
                            };
                            for (unsigned int si = 0;
                                 si < sizeof(sampleY) / sizeof(sampleY[0]);
                                 ++si) {
                                const int sy = sampleY[si];
                                const unsigned int addr =
                                    (lineAddress +
                                     static_cast<unsigned int>(sy) * 4u) &
                                    0x7FFFFu;
                                const unsigned int xsRaw =
                                    static_cast<unsigned int>(rawBytes[addr]) |
                                    (static_cast<unsigned int>(
                                        rawBytes[(addr + 1u) & 0x7FFFFu]) << 8);
                                const unsigned int xeRaw =
                                    static_cast<unsigned int>(
                                        rawBytes[(addr + 2u) & 0x7FFFFu]) |
                                    (static_cast<unsigned int>(
                                        rawBytes[(addr + 3u) & 0x7FFFFu]) << 8);
                                logging::writef(
                                    "[NeptuneVDP2] LW1 y=%d raw=%04X..%04X "
                                    "x=%u..%u\n",
                                    sy, xsRaw, xeRaw,
                                    (xsRaw >> 1) & 0x1FFu,
                                    (xeRaw >> 1) & 0x1FFu);
                            }
                            loggedD5RpWindow = true;
                        }

                        const float displayAspect =
                            static_cast<float>(viewerRenderWidth()) /
                            static_cast<float>(viewerRenderHeight());
                        const float xExtent =
                            (4.0f / 3.0f) / displayAspect;

                        unsigned int quadCount = 0u;
                        auto emitSpan =
                            [&](int y, int x0, int x1) {
                            if (quadCount >= 224u * 2u)
                                return;
                            x0 = std::clamp(x0, 0, 351);
                            x1 = std::clamp(x1, 0, 351);
                            if (x1 < x0)
                                return;

                            const float u0 =
                                static_cast<float>(x0) / 352.0f;
                            const float u1 =
                                static_cast<float>(x1 + 1) / 352.0f;
                            const float v0 =
                                static_cast<float>(y) / 224.0f;
                            const float v1 =
                                static_cast<float>(y + 1) / 224.0f;
                            const float px0 =
                                -xExtent + 2.0f * xExtent * u0;
                            const float px1 =
                                -xExtent + 2.0f * xExtent * u1;
                            const float py0 = 1.0f - 2.0f * v0;
                            const float py1 = 1.0f - 2.0f * v1;

                            const unsigned int base = quadCount * 4u;
                            g_vdp2WindowVertices[base + 0u] =
                                {px0, py0, 0.5f, u0, v0};
                            g_vdp2WindowVertices[base + 1u] =
                                {px1, py0, 0.5f, u1, v0};
                            g_vdp2WindowVertices[base + 2u] =
                                {px0, py1, 0.5f, u0, v1};
                            g_vdp2WindowVertices[base + 3u] =
                                {px1, py1, 0.5f, u1, v1};

                            const unsigned int ii = quadCount * 6u;
                            g_vdp2WindowIndices[ii + 0u] =
                                static_cast<std::uint16_t>(base + 0u);
                            g_vdp2WindowIndices[ii + 1u] =
                                static_cast<std::uint16_t>(base + 1u);
                            g_vdp2WindowIndices[ii + 2u] =
                                static_cast<std::uint16_t>(base + 2u);
                            g_vdp2WindowIndices[ii + 3u] =
                                static_cast<std::uint16_t>(base + 2u);
                            g_vdp2WindowIndices[ii + 4u] =
                                static_cast<std::uint16_t>(base + 1u);
                            g_vdp2WindowIndices[ii + 5u] =
                                static_cast<std::uint16_t>(base + 3u);
                            ++quadCount;
                        };

                        for (int y = 0; y < 224; ++y) {
                            const unsigned int addr =
                                (lineAddress +
                                 static_cast<unsigned int>(y) * 4u) &
                                0x7FFFFu;
                            const unsigned int xsRaw =
                                static_cast<unsigned int>(rawBytes[addr]) |
                                (static_cast<unsigned int>(
                                    rawBytes[(addr + 1u) & 0x7FFFFu]) << 8);
                            const unsigned int xeRaw =
                                static_cast<unsigned int>(
                                    rawBytes[(addr + 2u) & 0x7FFFFu]) |
                                (static_cast<unsigned int>(
                                    rawBytes[(addr + 3u) & 0x7FFFFu]) << 8);

                            int xs = 0;
                            int xe = 0;
                            if (xeRaw != 0xFFFFu) {
                                xs = static_cast<int>((xsRaw >> 1) & 0x1FFu);
                                xe = static_cast<int>((xeRaw >> 1) & 0x1FFu);
                            }

                            const bool yInside =
                                y >= yStart && y <= yEnd;
                            if (drawInside) {
                                if (yInside)
                                    emitSpan(y, xs, xe);
                            } else {
                                if (!yInside) {
                                    emitSpan(y, 0, 351);
                                } else {
                                    emitSpan(y, 0, xs - 1);
                                    emitSpan(y, xe + 1, 351);
                                }
                            }
                        }

                        if (quadCount) {
                            parameterAVertices = g_vdp2WindowVertices;
                            parameterAIndices = g_vdp2WindowIndices;
                            parameterAIndexCount = quadCount * 6u;
                        } else {
                            parameterAIndexCount = 0u;
                        }
                    }
                }

                if (rbgSubmitted && rpmd != 1u &&
                    parameterAIndexCount != 0u) {
                    rbgSubmitted = submitRbgParameter(
                        g_movieRbg0Planes,
                        g_movieRbg0TransformA,
                        g_movieRbg0CoefficientA,
                        0x1u, 0x2u, 0u,
                        parameterAVertices,
                        parameterAIndices,
                        parameterAIndexCount);
                }

                // RBG0 color offset is CLOFEN bit 4 on Saturn. D5 currently
                // reports CLOFEN=0x20, which is BACK, so do not incorrectly
                // tint the rotation plane when only the back screen is selected.
                if (rbgSubmitted) {
                    constexpr unsigned int kRbg0Bit = 0x10u;
                    drawAzelColorOffsetForLayer(kRbg0Bit, true);
                }

                // NBG uses the normal fullscreen front-end quad.
                sceGxmSetVertexStream(
                    g_probeContext, 0, g_movieVertices);
            }

            sceGxmSetFragmentProgram(
                g_probeContext, g_vdp2NbgFragmentProgram);
            void* nbgUniforms = nullptr;
            bool nbgSubmitted = false;
            if (sceGxmReserveFragmentDefaultUniformBuffer(
                    g_probeContext, &nbgUniforms) >= 0 &&
                nbgUniforms) {
                sceGxmSetUniformDataF(
                    nbgUniforms,
                    g_vdp2InfoParam,
                    0, 4, g_movieVdp2Info);
                nbgSubmitted = sceGxmDraw(
                    g_probeContext,
                    SCE_GXM_PRIMITIVE_TRIANGLES,
                    SCE_GXM_INDEX_FORMAT_U16,
                    g_movieIndices,
                    6) >= 0;
            }
            submitted = rbgSubmitted && nbgSubmitted;
        } else if (streamReady) {
            // Cinepak has already been reconstructed to a conventional RGBA
            // source-resolution texture above. Final presentation is now the
            // same cheap bilinear texture path as an ordinary decoded frame.
            sceGxmSetFragmentProgram(
                g_probeContext, g_movieTextureFragmentProgram);
            submitted = sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_movieIndices,
                6) >= 0;
        }
    }

    // Front-end text is decoded from Azel's NBG1/NBG3 VRAM/CRAM snapshot
    // into an ordinary RGBA texture. Draw it after the VDP2 backgrounds so
    // the existing LINEAR sampler handles presentation scaling in hardware.
    // VDP1 selectors/arrows remain above the text layer.
    if (submitted && g_movieUsesVdp2Title)
        drawAzelVdp2TextLayerGpu();

    if (submitted && g_movieUsesVdp2Title && g_movieVdp2Info[0] >= 0.5f)
        drawPublishedVdp1Ui();

    if (submitted) {
        const bool d5FrontEnd =
            g_movieUsesVdp2Title && g_movieVdp2Info[0] >= 0.5f;

        // D5 already received its hardware-accurate RBG0-only color offset
        // between the RBG0 and NBG passes above. Title and Cinepak/movie
        // surfaces instead represent the already-composited Saturn frontend,
        // so their Azel transition is a fullscreen compatibility pass.
        if (!d5FrontEnd)
            drawAzelFrontendFadeOffset();
    }

    const std::uint64_t gpuWaitStartUs =
        sceKernelGetProcessTimeWide();
    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    sceGxmFinish(g_probeContext);
    const unsigned int gpuWaitUs =
        static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - gpuWaitStartUs);
    const unsigned int frameRenderUs =
        static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - frameStartUs);

    static unsigned int frontendPerfFrames = 0u;
    if (g_movieUsesVdp2Title &&
        ((frontendPerfFrames++ % 60u) == 0u)) {
        logging::writef(
            "[VDP2Perf] fb=%ux%u frame=%uus gpuWait=%uus layout=%u\n",
            static_cast<unsigned int>(movieOutputWidth),
            static_cast<unsigned int>(movieOutputHeight),
            frameRenderUs,
            gpuWaitUs,
            static_cast<unsigned int>(g_movieVdp2Info[0]));
    }

    if (!submitted) {
        logging::writef("[MovieRender] FAIL movie draw submission\n");
        return true;
    }

    if (!g_movieRenderLogged) {
        logging::writef(
            "[MovieRender] first GXM frame submitted %ux%u output=%dx%d backend=%s msaa=OFF\n",
            g_movieWidth, g_movieHeight,
            movieOutputWidth, movieOutputHeight,
            g_movieUsesVdp2Title
                ? (g_movieVdp2Info[0] < 0.5f && g_titleDecodedValid
                    ? (highResolutionFrontend
                        ? "SGX-RGBA-TITLE-720X408"
                        : "SGX-RGBA-TITLE")
                    : "SGX-VDP2")
                : (g_movieUsesCinepakPayload ? "SGX-Cinepak-Linear" : "RGBA"));
        g_movieRenderLogged = true;
    }

    SceDisplayFrameBuf frameBuffer{};
    frameBuffer.size = sizeof(frameBuffer);
    frameBuffer.base = colorBuffer;
    frameBuffer.pitch = pitch;
    frameBuffer.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    frameBuffer.width = movieOutputWidth;
    frameBuffer.height = movieOutputHeight;

    if (!highResolutionFrontend) {
        waitFor30HzPresentSlot();
        sceDisplaySetFrameBuf(
            &frameBuffer, SCE_DISPLAY_SETBUF_NEXTFRAME);
        sceDisplayWaitVblankStart();
        mark30HzPresented();
    } else {
        // Saturn's high-resolution title advances its animation state at the
        // lower cadence even though the display is scanned more frequently.
        // Keep Neptune's cheap 720x408 render, but hold each published Azel
        // title state for two scanouts before returning the render slot.
        // Re-queueing the same completed buffer gives two display presents
        // without advancing the game/task graph twice.
        sceDisplaySetFrameBuf(
            &frameBuffer, SCE_DISPLAY_SETBUF_NEXTFRAME);
        sceDisplayWaitVblankStart();
        sceDisplaySetFrameBuf(
            &frameBuffer, SCE_DISPLAY_SETBUF_NEXTFRAME);
        sceDisplayWaitVblankStart();
        mark30HzPresented();
    }
    g_gxmDrawBuffer ^= 1;
    return true;
}

static void renderBasicWingViewer()
{
    if (renderMovieFrame())
        return;

    if (!g_viewerReady || !g_gxmInitialized || !g_probeContext ||
        !g_probeRenderTarget || !g_probeColorBuffer || !g_probeColorBuffer2 ||
        !g_probeVertexProgram || !g_probeFragmentProgram)
        return;

    // Resident VDP1 geometry is scene data, not renderer initialization state.
    // Authentic boot reaches the first native scene with no preloaded debug
    // model, so buildLiveTownFrame()/prepare_vdp1_model() must be allowed to
    // allocate these buffers on demand.

    static bool loggedSceneRendererEntry = false;
    if (!loggedSceneRendererEntry) {
        logging::writef(
            "[SceneRender] renderer entry viewer=%u gxm=%u context=%u\n",
            g_viewerReady ? 1u : 0u,
            g_gxmInitialized ? 1u : 0u,
            g_probeContext ? 1u : 0u);
        loggedSceneRendererEntry = true;
    }

    const std::uint64_t renderStartUs = sceKernelGetProcessTimeWide();
    g_profileBuildUs = 0u;
    g_profileLightingUs = 0u;
    g_profileSubmitUs = 0u;
    g_profileGxmWaitUs = 0u;
    g_liveTownGouraudPrepValid = false;

    // g_viewMode is part of the published game->render frame. The render
    // thread never reads mutable controller state directly.

    const bool nativeSceneMode =
        g_sceneGameMode != 0u && g_townCameraReady &&
        !azel_bridge::published_submissions().empty();
    const bool roomMode =
        g_staticRoomCpuReady || nativeSceneMode;
    const bool roomAuthenticCameraMode =
        nativeSceneMode ||
        (g_staticRoomCpuReady && g_staticRoomCpuMesh.cameraValid);

    // Legacy Basic Wing regression camera state is renderer-owned. Interactive
    // input is intentionally not sampled from this thread.

    const bool roomDiagnosticLitMode =
        g_staticRoomCpuReady &&
        g_staticRoomCpuMesh.lightingValid &&
        g_viewMode == 6;
    const bool liveSceneLighting =
        nativeSceneMode &&
        std::any_of(
            g_liveTownPolygonLights.begin(),
            g_liveTownPolygonLights.end(),
            [](const LivePolygonLightState& light) { return light.valid; });
    const bool roomAuthenticLitMode =
        roomAuthenticCameraMode &&
        (liveSceneLighting ||
         (g_staticRoomCpuReady && g_staticRoomCpuMesh.lightingValid)) &&
        g_viewMode == 7;
    const bool roomAuthenticTexturedMode =
        roomAuthenticCameraMode &&
        (g_viewMode == 8 ||
         (g_viewMode == 7 && !liveSceneLighting &&
          !(g_staticRoomCpuReady && g_staticRoomCpuMesh.lightingValid)));
    const bool roomAuthenticFlatMode =
        roomAuthenticCameraMode &&
        g_viewMode == 9;
    const bool roomAuthenticLightingOnlyMode =
        roomAuthenticCameraMode &&
        g_viewMode == 10;
    const bool roomAuthenticWireframeMode =
        roomAuthenticCameraMode &&
        g_viewMode == 11;

    if (!roomMode && g_residentVdp1Model != ResidentVdp1Model::BasicWing) {
        if (!prepare_vdp1_model(basicWingVdp1Source()))
            return;
        g_residentVdp1Model = ResidentVdp1Model::BasicWing;
        applyBasicWingAnimationFrame(g_basicWingAnimationFrame);
    } else if (roomAuthenticCameraMode) {
        const std::uint64_t buildStartUs = sceKernelGetProcessTimeWide();
        const bool liveTownBuilt = buildLiveTownFrame();
        g_profileBuildUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - buildStartUs);
        if (!liveTownBuilt)
            return;
    } else if (roomMode) {
        const ResidentVdp1Model desiredResident =
            ResidentVdp1Model::StaticRoomDiagnostic;

        if (g_residentVdp1Model != desiredResident) {
            if (!prepare_vdp1_model(staticRoomVdp1Source(false)))
                return;
            g_residentVdp1Model = desiredResident;
        }
    }

    if (!roomMode && g_basicWingCpuMesh.animationValid)
        advanceBasicWingAnimation();

    const ViewerMat4 wvp =
        roomAuthenticCameraMode
            ? buildAuthenticRoomWvp()
            : buildViewerWvp(roomMode);

    if (roomAuthenticCameraMode)
        prepareLiveTownGouraudVisibility(wvp);

    const int gxmPitch = viewerRenderPitch();

    std::uint32_t* const colorBuffer =
        g_gxmDrawBuffer == 0 ? g_probeColorBuffer : g_probeColorBuffer2;
    SceGxmColorSurface* const colorSurface =
        g_halfResolution
            ? (g_gxmDrawBuffer == 0
                ? &g_probeColorSurfaceHalf
                : &g_probeColorSurfaceHalf2)
            : (g_gxmDrawBuffer == 0
                ? &g_probeColorSurface
                : &g_probeColorSurface2);
    SceGxmSyncObject* const syncObject =
        g_gxmDrawBuffer == 0 ? g_probeSync : g_probeSync2;
    SceGxmRenderTarget* const renderTarget =
        g_halfResolution ? g_probeRenderTargetHalf : g_probeRenderTarget;
    SceGxmDepthStencilSurface* const depthSurface =
        g_halfResolution ? &g_probeDepthSurfaceHalf : &g_probeDepthSurface;

    const unsigned int alignedW =
        (viewerRenderWidth() + SCE_GXM_TILE_SIZEX - 1) &
        ~(SCE_GXM_TILE_SIZEX - 1);
    const unsigned int alignedH =
        (viewerRenderHeight() + SCE_GXM_TILE_SIZEY - 1) &
        ~(SCE_GXM_TILE_SIZEY - 1);

    std::memset(colorBuffer, 0,
                static_cast<std::size_t>(gxmPitch) *
                viewerRenderHeight() *
                sizeof(std::uint32_t));
    std::memset(g_probeDepth, 0xFF, alignedW * alignedH * 4u);
    std::memset(g_probeStencil, 0, alignedW * alignedH * 4u);

    if (sceGxmBeginScene(
            g_probeContext, 0, renderTarget,
            nullptr, nullptr, syncObject,
            colorSurface, depthSurface) < 0)
        return;

    Vdp1RenderMode renderMode = Vdp1RenderMode::PolygonColor;
    if (!roomMode) {
        renderMode = static_cast<Vdp1RenderMode>(g_viewMode);
    } else if (roomAuthenticFlatMode) {
        renderMode = Vdp1RenderMode::PolygonColor;
    } else if (roomAuthenticLightingOnlyMode) {
        renderMode = Vdp1RenderMode::GouraudGrayscale;
    } else if (roomAuthenticWireframeMode) {
        renderMode = Vdp1RenderMode::Wireframe;
    } else if (roomDiagnosticLitMode || roomAuthenticLitMode) {
        renderMode = Vdp1RenderMode::TexturedGouraud;
    } else if (g_staticRoomCpuMesh.texturesFullyResolved ||
               roomAuthenticTexturedMode) {
        renderMode = Vdp1RenderMode::Textured;
    }

    if (!roomMode &&
        (renderMode == Vdp1RenderMode::TexturedGouraud ||
         renderMode == Vdp1RenderMode::GouraudGrayscale))
        updateViewerAzelLighting();

    if (roomAuthenticCameraMode &&
        (roomAuthenticLitMode || roomAuthenticLightingOnlyMode)) {
        const std::uint64_t lightingStartUs = sceKernelGetProcessTimeWide();
        updateLiveTownAzelLighting();
        g_profileLightingUs = static_cast<unsigned int>(
            sceKernelGetProcessTimeWide() - lightingStartUs);
    } else if (roomDiagnosticLitMode) {
        updateStaticRoomAzelLighting(false);
    }

    Vdp1DrawState drawState{};
    std::memcpy(drawState.wvp, wvp.m, sizeof(drawState.wvp));
    drawState.mode = renderMode;
    // Town/room presentation uses the historical winding compensation for its
    // mirrored projection. Native field submissions arrive with the opposite
    // effective winding and are corrected separately at the rasterizer.
    drawState.reverseCullWinding = roomMode;

    const Vdp1ModelSource model =
        roomAuthenticCameraMode
            ? liveTownVdp1Source()
            : (roomMode
            ? staticRoomVdp1Source(false)
            : basicWingVdp1Source());

    static unsigned int shadowTraceHeartbeat = 0u;
    if (roomAuthenticCameraMode &&
        ((shadowTraceHeartbeat++ % 60u) == 0u)) {
        logging::writef(
            "[PresentationTrace][NeptuneScene] modelPolys=%u "
            "shadowReady=%u shadowTex=%u shadowFirst=%u shadowPolys=%u "
            "edgeFirst=%u edgePolys=%u resident=%u\n",
            static_cast<unsigned int>(model.polygonCount),
            g_edgeShadowCpuReady ? 1u : 0u,
            static_cast<unsigned int>(g_edgeShadowTownTextureIndices.size()),
            static_cast<unsigned int>(g_liveTownShadowFirstPolygon),
            static_cast<unsigned int>(g_liveTownShadowPolygonCount),
            static_cast<unsigned int>(g_liveTownEdgeFirstPolygon),
            static_cast<unsigned int>(g_liveTownEdgePolygonCount),
            static_cast<unsigned int>(g_residentVdp1Model));
    }

    const std::uint64_t submitStartUs = sceKernelGetProcessTimeWide();
    const bool submitted = submit_vdp1_model(model, drawState);
    g_profileSubmitUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - submitStartUs);
    if (!submitted) {
        sceGxmEndScene(g_probeContext, nullptr, nullptr);
        sceGxmFinish(g_probeContext);
        return;
    }

    if (roomAuthenticCameraMode) {
        // Saturn UI composition: NBG1 supplies window/backing tiles, the
        // line-scroll cinematic matte sits behind the glyph plane, and VDP1
        // sprites (including Lock-On and choice cursors) remain above VDP2.
        // Azel still owns the contents/state of every layer; Neptune only
        // translates their final presentation ordering to GXM.
        drawAzelVdp2Nbg1Gpu();
        drawAzelVdp2CinematicBarsGpu();
        drawAzelVdp2TextLayerGpu();
        drawPublishedVdp1Ui();
    }

    // The scripted full-screen fade tracked here belongs to the native
    // town camera adapter. Do not carry its terminal black state across a
    // module transition into field mode; field presentation follows Azel's
    // own VDP2/CLOFEN fade state instead.
    if (roomAuthenticCameraMode && g_sceneGameMode == 1u)
        drawFadeOverlay(updateTownFadeAlpha(), 0u, 0u, 0u);

    const std::uint64_t gxmWaitStartUs = sceKernelGetProcessTimeWide();
    const unsigned int renderCpuBeforeWaitUs =
        static_cast<unsigned int>(gxmWaitStartUs - renderStartUs);
    g_profileRenderCpuPrepUs =
        renderCpuBeforeWaitUs > g_profileSubmitUs
            ? renderCpuBeforeWaitUs - g_profileSubmitUs
            : 0u;
    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    sceGxmFinish(g_probeContext);
    g_profileGxmWaitUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - gxmWaitStartUs);

    g_profileRenderUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - renderStartUs);

    static unsigned int scenePerfHeartbeat = 0u;
    if (nativeSceneMode && ((scenePerfHeartbeat++ % 60u) == 0u)) {
        logging::writef(
            "[ScenePerf] build=%uus scan=%u cache=%u obj=%u edge=%u upload=%u "
            "light=%u submit=%u gxmwait=%u render=%u polys=%u verts=%u "
            "staticRebuilt=%u\n",
            g_profileBuildUs,
            g_profileBuildScanUs,
            g_profileBuildCacheUs,
            g_profileObjectAppendUs,
            g_profileBuildEdgeUs,
            g_profileBuildUploadUs,
            g_profileLightingUs,
            g_profileSubmitUs,
            g_profileGxmWaitUs,
            g_profileRenderUs,
            static_cast<unsigned>(g_liveTownCpuMesh.polygonRecords.size()),
            static_cast<unsigned>(g_liveTownCpuMesh.vertices.size()),
            g_liveTownStaticRebuilt ? 1u : 0u);
    }

    if (g_showThreadTimingOsd) {
        drawViewerModeOverlay(
            colorBuffer,
            gxmPitch,
            g_viewMode);
    }

    if (g_showThreadTimingOsd && roomAuthenticCameraMode) {
        char thread0[64], thread1[64], thread2[64], thread3[64];
        std::snprintf(
            thread0, sizeof(thread0),
            "GAME TASK %u WAIT %u",
            g_profileTasksUs,
            g_profileGameWaitUs);
        std::snprintf(
            thread1, sizeof(thread1),
            "REND PREP %u SUB %u",
            g_profileRenderCpuPrepUs,
            g_profileSubmitUs);
        std::snprintf(
            thread2, sizeof(thread2),
            "GPU WAIT %u PRES %u",
            g_profileGxmWaitUs,
            g_profilePresentUs);
        std::snprintf(
            thread3, sizeof(thread3),
            "REND TOTAL %u",
            g_profileRenderUs);

        const char* lines[4] = {thread0, thread1, thread2, thread3};
        constexpr int advance = 6;
        constexpr int xMargin = 8;
        constexpr int yStart = 8;
        constexpr int lineStep = 9;
        for (int line = 0; line < 4; ++line) {
            const int length =
                static_cast<int>(std::strlen(lines[line]));
            const int x = std::max(
                0,
                viewerRenderWidth() - xMargin - length * advance);
            const int y = yStart + line * lineStep;
            drawTextSmallToBuffer(
                colorBuffer, gxmPitch,
                x + 1, y + 1, lines[line], 0xFF000000u);
            drawTextSmallToBuffer(
                colorBuffer, gxmPitch,
                x, y, lines[line], 0xFFFFFFFFu);
        }
    }

    if (kShowTownDiagnostics) {
        drawTownInputOverlay(
            colorBuffer,
            gxmPitch,
            roomAuthenticCameraMode);
    }

    if (kShowTownDiagnostics && roomAuthenticFlatMode) {
        char perfLine[80];
        char cacheLine[80];
        char splitLine[80];

        std::snprintf(
            perfLine, sizeof(perfLine),
            "QUAD %u/%u DRAW %u",
            g_authFlatVisibleQuads,
            g_authFlatTotalQuads,
            g_authFlatDrawCalls);
        std::snprintf(
            cacheLine, sizeof(cacheLine),
            "SUB %u REBUILD %s BILL %s",
            g_liveTownSubmissionCount,
            g_liveTownStaticRebuilt ? "YES" : "NO",
            g_liveTownHasBillboards ? "YES" : "NO");
        std::snprintf(
            splitLine, sizeof(splitLine),
            "STATIC %u/%u BILL %u/%u",
            g_liveTownStaticSubmissionCount,
            g_liveTownStaticSubmittedPolygons,
            g_liveTownBillboardSubmissionCount,
            g_liveTownBillboardSubmittedPolygons);

        drawTextSmallToBuffer(
            colorBuffer, gxmPitch,
            8, 58,
            perfLine, 0xFFFFFFFFu);
        drawTextSmallToBuffer(
            colorBuffer, gxmPitch,
            8, 67,
            cacheLine, 0xFFFFFFFFu);
        drawTextSmallToBuffer(
            colorBuffer, gxmPitch,
            8, 76,
            splitLine, 0xFFFFFFFFu);
    }

    if (kShowTownDiagnostics && roomAuthenticCameraMode) {
        char timing0[80], timing1[80], timing2[80], timing3[80], timing4[80];
        char timing5[80], timing6[80], timing7[80], timing8[80];
        std::snprintf(
            timing0, sizeof(timing0),
            "US TASK %u BUILD %u LIGHT %u",
            g_profileTasksUs, g_profileBuildUs, g_profileLightingUs);
        std::snprintf(
            timing1, sizeof(timing1),
            "BUILD SCAN %u CACHE %u EDGE %u",
            g_profileBuildScanUs, g_profileBuildCacheUs, g_profileBuildEdgeUs);
        std::snprintf(
            timing2, sizeof(timing2),
            "EDGE COPY %u ANIM %u APP %u",
            g_profileEdgeCopyUs, g_profileEdgeAnimUs, g_profileEdgeAppendUs);
        std::snprintf(
            timing3, sizeof(timing3),
            "BUILD VAL %u UP %u SUB %u GXM %u",
            g_profileBuildValidateUs, g_profileBuildUploadUs,
            g_profileSubmitUs, g_profileGxmWaitUs);
        std::snprintf(
            timing4, sizeof(timing4),
            "US RENDER %u PRESENT %u",
            g_profileRenderUs, g_profilePresentUs);
        std::snprintf(
            timing5, sizeof(timing5),
            "GOUR PROJ %u PAY %u BUCKET %u",
            g_profileGouraudProjectUs,
            g_profileGouraudPayloadUs,
            g_profileGouraudBucketUs);
        std::snprintf(
            timing6, sizeof(timing6),
            "GOUR INDEX %u DRAW %u",
            g_profileGouraudIndexUs,
            g_profileGouraudDrawUs);
        std::snprintf(
            timing7, sizeof(timing7),
            "GOUR QUAD %u/%u PREP %u",
            g_profileGouraudVisibleQuads,
            g_profileGouraudTotalQuads,
            g_profileGouraudPrepUs);
        std::snprintf(
            timing8, sizeof(timing8),
            "OBJ APP %u MAT %u MISS %u",
            g_profileObjectAppendUs,
            g_profileObjectMaterialResolveUs,
            g_profileObjectMaterialCacheMisses);

        auto profileText = [colorBuffer, gxmPitch](
            int y, const char* text) {
            drawTextSmallToBuffer(
                colorBuffer, gxmPitch,
                9, y + 1, text, 0xFF000000u);
            drawTextSmallToBuffer(
                colorBuffer, gxmPitch,
                8, y, text, 0xFFFFFFFFu);
        };
        profileText(88, timing0);
        profileText(97, timing1);
        profileText(106, timing2);
        profileText(115, timing3);
        profileText(124, timing4);
        profileText(133, timing5);
        profileText(142, timing6);
        profileText(151, timing7);
        profileText(160, timing8);
    }

    if (g_showThreadTimingOsd) {
        const char* resolutionLabel =
            g_halfResolution ? "480X272 GXM" : "960X544 NATIVE";
        drawTextSmallToBuffer(
            colorBuffer,
            gxmPitch,
            17,
            viewerRenderHeight() - 11,
            resolutionLabel,
            0xFF000000u);
        drawTextSmallToBuffer(
            colorBuffer,
            gxmPitch,
            16,
            viewerRenderHeight() - 12,
            resolutionLabel,
            0xFFFFFFFFu);
    }

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = colorBuffer;
    fb.pitch = gxmPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = viewerRenderWidth();
    fb.height = viewerRenderHeight();

    const std::uint64_t presentStartUs = sceKernelGetProcessTimeWide();
    waitFor30HzPresentSlot();
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    mark30HzPresented();
    g_profilePresentUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - presentStartUs);

    // The buffer just queued is now front; draw the next frame into the
    // opposite GXM surface so scanout and rendering never touch the same
    // memory concurrently.
    g_gxmDrawBuffer ^= 1;
}

static int renderThreadMain(SceSize, void*)
{
    while (true) {
        if (sceKernelWaitSema(g_renderFrameReadySema, 1, nullptr) < 0)
            break;
        if (!g_renderThreadRunning)
            break;

        if (g_renderStartupFrame < 3)
            logging::writef(
                "[RenderFrame] %u begin\n",
                g_renderStartupFrame);
        if (!g_debugVisible)
            renderBasicWingViewer();
        if (g_renderStartupFrame < 3)
            logging::writef(
                "[RenderFrame] %u end\n",
                g_renderStartupFrame);
        ++g_renderStartupFrame;

        // The published bridge/presentation state may now be overwritten by
        // the game thread for the next completed frame.
        sceKernelSignalSema(g_renderFrameFreeSema, 1);
    }
    return 0;
}

void begin_frame()
{
    if (!g_debugVisible) {
        // Controller state belongs to the game thread. Only the selected view
        // is copied into the next published render frame.
        static constexpr int kSceneModes[5] = {7, 8, 10, 9, 11};
        int sceneModeIndex = 0;
        for (int i = 0; i < 5; ++i)
            if (g_pendingViewMode == kSceneModes[i])
                sceneModeIndex = i;

        if (input::prev_mode_pressed())
            sceneModeIndex = (sceneModeIndex + 4) % 5;
        if (input::next_mode_pressed())
            sceneModeIndex = (sceneModeIndex + 1) % 5;
        g_pendingViewMode = kSceneModes[sceneModeIndex];
        return;
    }

    fill(0xFF000000u);

    if (g_gxmProbeAttempted && g_viewerReady) {
        // Start+Select exposes the retained bring-up/status screen without
        // stopping the Azel task graph. The game continues to simulate while
        // this CPU-side diagnostic framebuffer is presented.
        drawTextSmall(40, 18, "LAGI DEBUG STATUS", 0xFFFFFFFFu);
        drawTextSmall(
            40, 34,
            "START+SELECT: RETURN   SELECT: PERFORMANCE OSD",
            0xFFB0B0B0u);

        for (int i = 0; i < g_statusCount; ++i) {
            const int column = i / kStatusRowsPerColumn;
            const int row = i % kStatusRowsPerColumn;
            if (column >= 2)
                break;
            drawTextSmall(
                kStatusColumnX[column],
                58 + row * kStatusLineHeight,
                g_status[i].text,
                g_status[i].color);
        }
        return;
    }

    // Normal startup presentation stays intentionally minimal. Detailed boot
    // status is still collected/logged, but the user sees only a black screen
    // with a small loading indicator until the first completed town frame is
    // handed to GXM. That first frame is itself held black by TwnFadeIn state.
    static constexpr const char* kLoadingText = "Loading...";
    static constexpr int kLoadingAdvance = 6;
    static constexpr int kLoadingMarginX = 16;
    static constexpr int kLoadingMarginY = 14;
    const int loadingLength =
        static_cast<int>(std::strlen(kLoadingText));
    drawTextSmall(
        kWidth - kLoadingMarginX - loadingLength * kLoadingAdvance,
        kHeight - kLoadingMarginY,
        kLoadingText,
        0xFFFFFFFFu);
}

void end_frame()
{
    if (!g_debugVisible)
        return;

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = g_frameBuffer[g_drawBuffer];
    fb.pitch = kPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    g_drawBuffer ^= 1;
}

static void updateLiveTownAzelLighting()
{
    if (g_liveTownCpuMesh.gouraud555.size() !=
            g_liveTownCpuMesh.polygonRecords.size() ||
        g_liveTownPolygonLights.size() !=
            g_liveTownCpuMesh.polygonRecords.size())
        return;

    const bool usePreparedVisibility =
        g_liveTownGouraudPrepValid &&
        g_liveTownGouraudPrepVertices == g_liveTownCpuMesh.vertices.data() &&
        g_liveTownGouraudPrepPolygonCount ==
            g_liveTownCpuMesh.polygonRecords.size() &&
        g_liveTownGouraudPrep.size() ==
            g_liveTownCpuMesh.polygonRecords.size();

    float cameraForward[3] = {
        g_townCameraTarget[0] - g_townCameraPosition[0],
        g_townCameraTarget[1] - g_townCameraPosition[1],
        g_townCameraTarget[2] - g_townCameraPosition[2]};
    const float length = std::sqrt(
        cameraForward[0]*cameraForward[0] +
        cameraForward[1]*cameraForward[1] +
        cameraForward[2]*cameraForward[2]);
    if (length > 0.000001f)
        for (float& v : cameraForward) v /= length;

    constexpr std::int64_t farRaw = 0xF000;
    constexpr std::int64_t oneOverFar =
        (static_cast<std::int64_t>(0x8000) << 16) / farRaw;
    constexpr std::int64_t oneOverFar256 = oneOverFar << 8;
    auto falloffIndex = [&](std::size_t polygon) {
        const auto& v = g_liveTownCpuMesh.vertices[polygon * 6u];
        const float depth = std::fabs(
            (v.x - g_townCameraPosition[0]) * cameraForward[0] +
            (v.y - g_townCameraPosition[1]) * cameraForward[1] +
            (v.z - g_townCameraPosition[2]) * cameraForward[2]);
        const std::int64_t viewDepth =
            static_cast<std::int64_t>(std::llround(depth * 65536.0f)) << 8;
        const std::int64_t scaled = std::max<std::int64_t>(
            0, (viewDepth * oneOverFar256) >> 32);
        const int byteOffset =
            (static_cast<int>((scaled << 1) >> 8)) & ~7;
        return std::clamp(byteOffset >> 3, 0, 31);
    };

    std::uint32_t cachedFalloff[3]{0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    std::int16_t falloffMap[32][3]{};
    bool haveFalloff = false;
    static bool reportedLiveLighting = false;

    for (std::size_t p = 0;
         p < g_liveTownCpuMesh.polygonRecords.size(); ++p) {
        if (usePreparedVisibility && !g_liveTownGouraudPrep[p].visible)
            continue;

        const auto& record = g_liveTownCpuMesh.polygonRecords[p];
        const auto& light = g_liveTownPolygonLights[p];
        const unsigned mode = (record.lightingControl >> 8) & 3u;
        auto& out = g_liveTownCpuMesh.gouraud555[p];
        out = {};
        if (!light.valid || mode == 0u || record.lightingCount == 0u)
            continue;

        if (!haveFalloff ||
            cachedFalloff[0] != light.falloff[0] ||
            cachedFalloff[1] != light.falloff[1] ||
            cachedFalloff[2] != light.falloff[2]) {
            cachedFalloff[0] = light.falloff[0];
            cachedFalloff[1] = light.falloff[1];
            cachedFalloff[2] = light.falloff[2];
            generateAzelFalloff(
                cachedFalloff[0], cachedFalloff[1], cachedFalloff[2],
                falloffMap);
            haveFalloff = true;
        }

        if (!reportedLiveLighting &&
            (light.vector[0] != 0 || light.vector[1] != 0 ||
             light.vector[2] != 0 || light.color[0] != 0 ||
             light.color[1] != 0 || light.color[2] != 0)) {
            logging::writef(
                "[SceneLight] active light vec=(%d,%d,%d) rgb=(%u,%u,%u) "
                "falloff=%08X/%08X/%08X\n",
                light.vector[0], light.vector[1], light.vector[2],
                static_cast<unsigned>(light.color[0]),
                static_cast<unsigned>(light.color[1]),
                static_cast<unsigned>(light.color[2]),
                static_cast<unsigned>(light.falloff[0]),
                static_cast<unsigned>(light.falloff[1]),
                static_cast<unsigned>(light.falloff[2]));
            reportedLiveLighting = true;
        }

        const int depthIndex = falloffIndex(p);
        for (unsigned corner = 0; corner < 4u; ++corner) {
            const unsigned normalIndex = mode == 1u ? 0u : corner;
            if (normalIndex >= record.lightingCount)
                continue;

            const auto& lighting = record.lighting[normalIndex];
            const int dotProduct =
                static_cast<int>(lighting.normal[0]) * light.vector[0] +
                static_cast<int>(lighting.normal[1]) * light.vector[1] +
                static_cast<int>(lighting.normal[2]) * light.vector[2];

            int accum[3] = {
                falloffMap[depthIndex][0],
                falloffMap[depthIndex][1],
                falloffMap[depthIndex][2]};

            if (mode == 2u && lighting.hasColor) {
                for (unsigned channel = 0; channel < 3; ++channel)
                    accum[channel] += static_cast<std::int16_t>(
                        lighting.color[channel]);
            }

            if (dotProduct > 0) {
                const int dotHi = static_cast<int>(
                    static_cast<std::uint32_t>(dotProduct) >> 16);
                for (unsigned channel = 0; channel < 3; ++channel)
                    accum[channel] += light.color[channel] * dotHi;
            }

            for (unsigned channel = 0; channel < 3; ++channel) {
                const int gouraud5 =
                    (std::clamp(accum[channel], 0, 0x1F00) >> 8) & 0x1F;
                out.corner[corner][channel] =
                    (static_cast<float>(gouraud5) - 16.0f) / 31.0f;
            }
        }
    }
}

bool presentation_active()
{
    const bool sceneMode =
        g_pendingViewMode == 7 || g_pendingViewMode == 8 ||
        g_pendingViewMode == 10 || g_pendingViewMode == 9 ||
        g_pendingViewMode == 11;
    const bool nativeSceneReady =
        g_sceneGameMode != 0u && g_townCameraReady;
    const bool legacyRoomReady =
        g_staticRoomCpuReady && g_staticRoomCpuMesh.cameraValid;
    return !g_debugVisible && sceneMode &&
           (nativeSceneReady || legacyRoomReady);
}

void presentation_profile_tasks_us(unsigned int microseconds)
{
    g_pendingProfileTasksUs = microseconds;
}

void presentation_wait_frame_slot()
{
    const std::uint64_t waitStartUs = sceKernelGetProcessTimeWide();
    if (g_renderThreadStarted && g_renderFrameFreeSema >= 0)
        sceKernelWaitSema(g_renderFrameFreeSema, 1, nullptr);
    g_pendingProfileGameWaitUs = static_cast<unsigned int>(
        sceKernelGetProcessTimeWide() - waitStartUs);
}

void presentation_publish_frame()
{
    if (!g_pendingTownPresentationValid) {
        // presentation_wait_frame_slot() has already consumed the producer token.
        // Azel is allowed to spend startup frames without publishing Edge or
        // camera state, so return that token when there is no frame to queue.
        // Otherwise the next game frame waits forever and presentation stays
        // black even though the process itself is still alive.
        if (g_renderThreadStarted && g_renderFrameFreeSema >= 0)
            sceKernelSignalSema(g_renderFrameFreeSema, 1);
        return;
    }

    std::memcpy(
        g_townPlayerPosition,
        g_pendingTownPlayerPosition,
        sizeof(g_townPlayerPosition));
    g_sceneGameMode = g_pendingSceneGameMode;
    g_townPlayerYaw = g_pendingTownPlayerYaw;
    g_townPlayerGrounded = g_pendingTownPlayerGrounded;
    g_townCollisionContacts = g_pendingTownCollisionContacts;
    g_townEdgeAnimation = g_pendingTownEdgeAnimation;
    g_townEdgeAnimationFrame = g_pendingTownEdgeAnimationFrame;
    g_townEdgePreviousAnimation = g_pendingTownEdgePreviousAnimation;
    g_townEdgePreviousFrame = g_pendingTownEdgePreviousFrame;
    g_townEdgeTransition = g_pendingTownEdgeTransition;

    std::memcpy(
        g_townCameraPosition,
        g_pendingTownCameraPosition,
        sizeof(g_townCameraPosition));
    std::memcpy(
        g_townCameraRawPosition,
        g_pendingTownCameraRawPosition,
        sizeof(g_townCameraRawPosition));
    std::memcpy(
        g_townCameraTarget,
        g_pendingTownCameraTarget,
        sizeof(g_townCameraTarget));
    std::memcpy(
        g_townCameraUp,
        g_pendingTownCameraUp,
        sizeof(g_townCameraUp));
    g_townCameraYaw = g_pendingTownCameraYaw;
    g_townCameraPitch = g_pendingTownCameraPitch;
    g_townCameraDistance = g_pendingTownCameraDistance;
    g_nativeSceneNearPlane = g_pendingNativeSceneNearPlane;
    g_nativeSceneFarPlane = g_pendingNativeSceneFarPlane;
    g_viewMode = g_pendingViewMode;
    g_profileTasksUs = g_pendingProfileTasksUs;
    g_profileGameWaitUs = g_pendingProfileGameWaitUs;
    g_townFadeSerial = g_pendingTownFadeSerial;
    g_townFadeIn = g_pendingTownFadeIn;
    g_townFadeFrames = g_pendingTownFadeFrames;
    if (g_pendingVdp2TextValid) {
        std::memcpy(
            g_vdp2TextVram,
            g_pendingVdp2TextVram,
            sizeof(g_vdp2TextVram));
        std::memcpy(
            g_vdp2Cram,
            g_pendingVdp2Cram,
            sizeof(g_vdp2Cram));
        std::memcpy(
            g_vdp2LineScroll,
            g_pendingVdp2LineScroll,
            sizeof(g_vdp2LineScroll));
        g_vdp2TextValid = true;
    }

    static unsigned int presentationTraceHeartbeat = 0u;
    if ((presentationTraceHeartbeat++ % 60u) == 0u) {
        unsigned int publishedNbg1Cells = 0u;
        unsigned int publishedTextCells = 0u;
        if (g_vdp2TextValid) {
            constexpr unsigned int kNbg1MapOffset = 0x5800u;
            constexpr unsigned int kNbg1Cells = 32u * 14u;
            constexpr unsigned int kTextMapOffset = 0x6000u;
            constexpr unsigned int kTextCells = 64u * 28u;
            for (unsigned int i = 0; i < kNbg1Cells; ++i) {
                if (readVdp2Be16(
                        g_vdp2TextVram,
                        kNbg1MapOffset + i * 2u) != 0u)
                    ++publishedNbg1Cells;
            }
            for (unsigned int i = 0; i < kTextCells; ++i) {
                if (readVdp2Be16(
                        g_vdp2TextVram,
                        kTextMapOffset + i * 2u) != 0u)
                    ++publishedTextCells;
            }
        }

        logging::writef(
            "[PresentationTrace][Publish] pendingVDP2=%u publishedVDP2=%u "
            "nbg1Cells=%u textCells=%u uiCmds=%u shadowPolys=%u edgePolys=%u\n",
            g_pendingVdp2TextValid ? 1u : 0u,
            g_vdp2TextValid ? 1u : 0u,
            publishedNbg1Cells,
            publishedTextCells,
            static_cast<unsigned int>(
                azel_bridge::published_vdp1_ui_commands().size()),
            static_cast<unsigned int>(g_liveTownShadowPolygonCount),
            static_cast<unsigned int>(g_liveTownEdgePolygonCount));
    }

    if (g_renderThreadStarted && g_renderFrameReadySema >= 0)
        sceKernelSignalSema(g_renderFrameReadySema, 1);
}

void presentation_fade_in(unsigned int frames)
{
    g_pendingTownFadeIn = true;
    g_pendingTownFadeFrames = std::max(1u, frames);
    ++g_pendingTownFadeSerial;
    logging::writef(
        "[Presentation] FadeIn frames=%u\n",
        g_pendingTownFadeFrames);
}

void presentation_fade_out(unsigned int frames)
{
    g_pendingTownFadeIn = false;
    g_pendingTownFadeFrames = std::max(1u, frames);
    ++g_pendingTownFadeSerial;
    logging::writef(
        "[Presentation] FadeOut frames=%u\n",
        g_pendingTownFadeFrames);
}

void presentation_camera_update()
{
    // Native scene pose/camera state is consumed at the publish boundary.
}

void presentation_set_scene_mode(unsigned int gameMode)
{
    g_pendingSceneGameMode = gameMode;
    g_pendingTownPresentationValid = true;
}

void presentation_set_clip_planes(float nearPlane, float farPlane)
{
    if (nearPlane > 0.0f && farPlane > nearPlane) {
        g_pendingNativeSceneNearPlane = nearPlane;
        g_pendingNativeSceneFarPlane = farPlane;
        g_pendingTownPresentationValid = true;
    }
}

unsigned presentation_player_animation_frames(unsigned animation)
{
    if (animation >= g_edgeIdleCpuMesh.edgeAnimationClips.size()) return 0;
    const auto& clip = g_edgeIdleCpuMesh.edgeAnimationClips[animation];
    return clip.valid ? static_cast<unsigned>(clip.frames.size()) : 0;
}

void presentation_set_player(
    float x, float y, float z, float yaw,
    bool grounded, unsigned contacts,
    unsigned animation, unsigned frame,
    unsigned previousAnimation, unsigned previousFrame,
    float transition)
{
    g_townPlayerReady = true;
    g_pendingTownPlayerPosition[0] = x;
    g_pendingTownPlayerPosition[1] = y;
    g_pendingTownPlayerPosition[2] = z;
    g_pendingTownPlayerYaw = yaw;
    g_pendingTownPlayerGrounded = grounded;
    g_pendingTownCollisionContacts = contacts;
    g_pendingTownEdgeAnimation = animation;
    g_pendingTownEdgeAnimationFrame = frame;
    g_pendingTownEdgePreviousAnimation = previousAnimation;
    g_pendingTownEdgePreviousFrame = previousFrame;
    g_pendingTownEdgeTransition = transition;
    g_pendingTownPresentationValid = true;
}

void presentation_set_camera(
    const float position[3],
    const float rawPosition[3],
    const float target[3],
    const float up[3],
    float yaw, float pitch, float distance)
{
    const float dx = target[0] - position[0];
    const float dy = target[1] - position[1];
    const float dz = target[2] - position[2];
    const float viewLenSq = dx*dx + dy*dy + dz*dz;
    const float ux = up[0] - position[0];
    const float uy = up[1] - position[1];
    const float uz = up[2] - position[2];
    const float upLenSq = ux*ux + uy*uy + uz*uz;
    const bool cameraReady =
        viewLenSq > 0.000001f && upLenSq > 0.000001f;

    static bool loggedValidCamera = false;
    static unsigned int invalidCameraSamples = 0u;
    if (!cameraReady) {
        if (invalidCameraSamples < 4u) {
            logging::writef(
                "[PresentationCamera] invalid sample=%u "
                "pos=(%.5f,%.5f,%.5f) raw=(%.5f,%.5f,%.5f) "
                "target=(%.5f,%.5f,%.5f) up=(%.5f,%.5f,%.5f)\n",
                invalidCameraSamples,
                position[0], position[1], position[2],
                rawPosition[0], rawPosition[1], rawPosition[2],
                target[0], target[1], target[2],
                up[0], up[1], up[2]);
        }
        ++invalidCameraSamples;
        g_townCameraReady = false;
        return;
    }

    g_townCameraReady = true;
    if (!loggedValidCamera) {
        logging::writef(
            "[PresentationCamera] valid "
            "pos=(%.5f,%.5f,%.5f) target=(%.5f,%.5f,%.5f) "
            "up=(%.5f,%.5f,%.5f)\n",
            position[0], position[1], position[2],
            target[0], target[1], target[2],
            up[0], up[1], up[2]);
        loggedValidCamera = true;
    }

    std::memcpy(
        g_pendingTownCameraPosition,
        position,
        sizeof(g_pendingTownCameraPosition));
    std::memcpy(
        g_pendingTownCameraRawPosition,
        rawPosition,
        sizeof(g_pendingTownCameraRawPosition));
    std::memcpy(
        g_pendingTownCameraTarget,
        target,
        sizeof(g_pendingTownCameraTarget));
    std::memcpy(
        g_pendingTownCameraUp,
        up,
        sizeof(g_pendingTownCameraUp));
    g_pendingTownCameraYaw = yaw;
    g_pendingTownCameraPitch = pitch;
    g_pendingTownCameraDistance = distance;
    g_pendingTownPresentationValid = true;
}

void presentation_set_vdp2_text(
    const unsigned char* vram,
    const unsigned char* cram,
    const unsigned char* lineScroll)
{
    if (!vram || !cram || !lineScroll) {
        g_pendingVdp2TextValid = false;
        return;
    }

    std::memcpy(
        g_pendingVdp2TextVram,
        vram,
        sizeof(g_pendingVdp2TextVram));
    std::memcpy(
        g_pendingVdp2Cram,
        cram,
        sizeof(g_pendingVdp2Cram));
    std::memcpy(
        g_pendingVdp2LineScroll,
        lineScroll,
        sizeof(g_pendingVdp2LineScroll));
    g_pendingVdp2TextValid = true;

    static unsigned int setVdp2TraceHeartbeat = 0u;
    if ((setVdp2TraceHeartbeat++ % 60u) == 0u) {
        logging::writef(
            "[PresentationTrace][SetVDP2] snapshot staged vram=%u cram=%u lineScroll=%u\n",
            static_cast<unsigned int>(sizeof(g_pendingVdp2TextVram)),
            static_cast<unsigned int>(sizeof(g_pendingVdp2Cram)),
            static_cast<unsigned int>(sizeof(g_pendingVdp2LineScroll)));
    }
}

} // namespace lagi::platform::renderer
