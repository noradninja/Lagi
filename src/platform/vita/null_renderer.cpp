#include "lagi/platform.h"
#include "lagi/debug_mesh.h"
#include "lagi/vdp1_renderer.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <vector>

namespace lagi::platform::renderer {

extern "C" {
extern const unsigned char _binary_lagi_color_v_gxp_start[];
extern const unsigned char _binary_lagi_color_f_gxp_start[];
extern const unsigned char _binary_lagi_texture_v_gxp_start[];
extern const unsigned char _binary_lagi_texture_f_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_payload_v_gxp_start[];
extern const unsigned char _binary_lagi_gouraud_debug_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_lit_f_gxp_start[];
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
static SceUID g_probeColorUid = -1;
static std::uint32_t* g_probeColorBuffer = nullptr;
static SceGxmColorSurface g_probeColorSurface{};
static SceGxmColorSurface g_probeColorSurfaceHalf{};
static SceGxmSyncObject* g_probeSync = nullptr;
static SceUID g_probeColorUid2 = -1;
static std::uint32_t* g_probeColorBuffer2 = nullptr;
static SceGxmColorSurface g_probeColorSurface2{};
static SceGxmColorSurface g_probeColorSurfaceHalf2{};
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
static const SceGxmProgramParameter* g_probeWvpParam = nullptr;

static SceGxmShaderPatcherId g_textureVertexProgramId{};
static SceGxmShaderPatcherId g_textureFragmentProgramId{};
static bool g_textureVertexRegistered = false;
static bool g_textureFragmentRegistered = false;
static SceGxmVertexProgram* g_textureVertexProgram = nullptr;
static SceGxmFragmentProgram* g_textureFragmentProgram = nullptr;
static SceGxmShaderPatcherId g_gouraudPayloadVertexProgramId{};
static bool g_gouraudPayloadVertexRegistered = false;
static SceGxmVertexProgram* g_gouraudPayloadVertexProgram = nullptr;
static const SceGxmProgramParameter* g_gouraudPayloadWvpParam = nullptr;

static SceGxmShaderPatcherId g_gouraudDebugFragmentProgramId{};
static bool g_gouraudDebugFragmentRegistered = false;
static SceGxmFragmentProgram* g_gouraudDebugFragmentProgram = nullptr;

static SceGxmShaderPatcherId g_texturedLitFragmentProgramId{};
static bool g_texturedLitFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedLitFragmentProgram = nullptr;

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

enum class ResidentVdp1Model {
    None,
    BasicWing,
    StaticRoomDiagnostic,
    StaticRoomAuthentic,
};
static ResidentVdp1Model g_residentVdp1Model = ResidentVdp1Model::None;
static SceUID g_vdp1VertexUid = -1;
static SceUID g_vdp1LightingVertexUid = -1;
static SceUID g_vdp1IndexUid = -1;
static azel::DebugColorVertex* g_vdp1Vertices = nullptr;
static azel::DebugColorVertex* g_vdp1LightingVertices = nullptr;
static std::uint16_t* g_vdp1Indices = nullptr;

static SceUID g_vdp1TextureVertexUid = -1;
static SceUID g_vdp1GouraudVertexUid = -1;
static SceUID g_vdp1TextureIndexUid = -1;
static azel::DebugTextureVertex* g_vdp1TextureVertices = nullptr;
static azel::DebugGouraudPayloadVertex* g_vdp1GouraudVertices = nullptr;
static std::uint16_t* g_vdp1TextureIndices = nullptr;

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
};

static std::vector<TextureBatch> g_vdp1TextureBatches;
static std::vector<GpuMode1Texture> g_vdp1GpuTextures;
static bool g_vdp1TexturedReady = false;

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
static float g_townPlayerPosition[3]{};
static float g_townPlayerStartPosition[3]{};
static float g_townPlayerYaw = 0.0f;
static float g_townPlayerStartYaw = 0.0f;
static float g_townCameraPosition[3]{};
static float g_townCameraTarget[3]{};
static float g_townCameraUp[3]{};
static float g_townCameraStartOffset[3]{};
static float g_townTargetStartOffset[3]{};

static bool g_staticRoomAzelCellVisible = true;
static unsigned int g_staticRoomAzelLod0Objects = 0;
static unsigned int g_staticRoomAzelNonzeroLodObjects = 0;

static int g_viewMode = 0;
static bool g_halfResolution = true;
static unsigned int g_basicWingAnimationFrame = 0;
static std::uint64_t g_basicWingAnimationLastUs = 0;
static std::uint64_t g_basicWingAnimationPhase = 0;

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
    static const char* labels[] = {
        "MODE 0 - BASIC WING TEXTURED",
        "MODE 1 - BASIC WING GOURAUD",
        "MODE 2 - BASIC WING GOURAUD GRAY",
        "MODE 3 - BASIC WING POLY DEBUG",
        "MODE 4 - BASIC WING WIREFRAME",
        "MODE 5 - RUIN TEXTURED",
        "MODE 6 - RUIN GOURAUD",
        "MODE 7 - AUTH GOURAUD",
        "MODE 8 - AUTH TEXTURED",
        "MODE 9 - AUTH FLAT",
        "MODE 10 - AUTH LIGHTING ONLY"
    };

    if (mode < 0 ||
        mode >= static_cast<int>(
            sizeof(labels) / sizeof(labels[0])))
        return "MODE ?";

    return labels[mode];
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
        fill(0xFF181818u);
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
    return true;
}

void shutdown()
{
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
    freeVdp1Textures();

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
        if (g_textureFragmentProgram) {
            sceGxmShaderPatcherReleaseFragmentProgram(
                g_probeShaderPatcher, g_textureFragmentProgram);
            g_textureFragmentProgram = nullptr;
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
        if (g_textureFragmentRegistered) {
            sceGxmShaderPatcherUnregisterProgram(
                g_probeShaderPatcher, g_textureFragmentProgramId);
            g_textureFragmentRegistered = false;
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

    void* colorPtr2 = g_probeColorBuffer2;
    freeProbeMapped(g_probeColorUid2, colorPtr2);
    g_probeColorBuffer2 = nullptr;

    void* colorPtr = g_probeColorBuffer;
    freeProbeMapped(g_probeColorUid, colorPtr);
    g_probeColorBuffer = nullptr;
    freeProbeMapped(g_probeDepthUid, g_probeDepth);
    freeProbeMapped(g_probeStencilUid, g_probeStencil);

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

static bool buildVdp1TexturedBuffers(const Vdp1ModelSource& model)
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

    if (!g_vdp1TextureVertices ||
        !g_vdp1GouraudVertices ||
        !g_vdp1TextureIndices)
        return false;

    static const int triCorners[6] = {0, 1, 2, 0, 2, 3};

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

    unsigned int outIndex = 0;
    g_vdp1TextureBatches.assign(
        g_vdp1GpuTextures.size(), TextureBatch{});

    for (unsigned int t = 0;
         t < g_vdp1GpuTextures.size(); ++t) {
        TextureBatch& batch = g_vdp1TextureBatches[t];
        batch.firstIndex = outIndex;

        for (unsigned int p = 0;
             p < model.polygonCount; ++p) {
            if (model.polygonTextureIndices[p] != t)
                continue;

            for (unsigned int k = 0; k < 6; ++k)
                g_vdp1TextureIndices[outIndex++] =
                    static_cast<std::uint16_t>(p * 6u + k);
        }

        batch.indexCount = outIndex - batch.firstIndex;
    }

    return outIndex == vertexCount;
}

static void releaseResidentVdp1Model()
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

    p = g_vdp1TextureVertices;
    freeMapped(g_vdp1TextureVertexUid, p);
    g_vdp1TextureVertices = nullptr;

    p = g_vdp1GouraudVertices;
    freeMapped(g_vdp1GouraudVertexUid, p);
    g_vdp1GouraudVertices = nullptr;

    p = g_vdp1TextureIndices;
    freeMapped(g_vdp1TextureIndexUid, p);
    g_vdp1TextureIndices = nullptr;

    freeVdp1Textures();
    g_vdp1TextureBatches.clear();
    g_vdp1TexturedReady = false;
    g_residentVdp1Model = ResidentVdp1Model::None;
}

bool prepare_vdp1_model(const Vdp1ModelSource& model)
{
    if (!g_gxmInitialized || !g_probeContext || !model.valid())
        return false;

    // The viewer still keeps one VDP1 model resident at a time, but M3 can
    // now swap between the Basic Wing regression mesh and the reconstructed
    // first-room diagnostic mesh.
    if (g_vdp1Vertices || g_vdp1LightingVertices || g_vdp1Indices ||
        g_vdp1TextureVertices || g_vdp1GouraudVertices ||
        g_vdp1TextureIndices || !g_vdp1GpuTextures.empty())
        releaseResidentVdp1Model();

    if (model.vertexCount > 65535u)
        return false;

    const unsigned int vertexCount =
        static_cast<unsigned int>(model.vertexCount);
    const unsigned int vertexBytes =
        vertexCount * sizeof(azel::DebugColorVertex);
    const unsigned int indexBytes =
        vertexCount * sizeof(std::uint16_t);

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

    if (!g_vdp1Vertices ||
        !g_vdp1LightingVertices ||
        !g_vdp1Indices)
        return false;

    if (model.texturesValid()) {
        if (!uploadVdp1Textures(model) ||
            !buildVdp1TexturedBuffers(model))
            return false;
        g_vdp1TexturedReady = true;
    } else {
        g_vdp1TexturedReady = false;
    }

    std::memcpy(g_vdp1Vertices, model.vertices, vertexBytes);
    std::memcpy(g_vdp1LightingVertices, model.lightingVertices, vertexBytes);
    for (unsigned int i = 0; i < vertexCount; ++i)
        g_vdp1Indices[i] = static_cast<std::uint16_t>(i);

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

    // Keep PDS animation on an independent 30 Hz clock instead of tying it
    // to rendered frames. sceKernelGetProcessTimeWide() is microseconds.
    //
    // Accumulating elapsed_us * 30 against 1,000,000 avoids the small drift
    // that would come from treating one tick as an integer 33,333 us.
    const std::uint64_t nowUs =
        static_cast<std::uint64_t>(
            sceKernelGetProcessTimeWide());

    if (g_basicWingAnimationLastUs == 0) {
        g_basicWingAnimationLastUs = nowUs;
        applyBasicWingAnimationFrame(
            g_basicWingAnimationFrame);
        return;
    }

    const std::uint64_t elapsedUs =
        nowUs - g_basicWingAnimationLastUs;
    g_basicWingAnimationLastUs = nowUs;

    constexpr std::uint64_t kAnimationHz = 30u;
    constexpr std::uint64_t kMicrosecondsPerSecond = 1000000u;

    g_basicWingAnimationPhase +=
        elapsedUs * kAnimationHz;

    const std::uint64_t elapsedTicks =
        g_basicWingAnimationPhase /
        kMicrosecondsPerSecond;
    g_basicWingAnimationPhase %=
        kMicrosecondsPerSecond;

    if (elapsedTicks == 0)
        return;

    const std::uint64_t frameCount =
        static_cast<std::uint64_t>(
            g_basicWingCpuMesh.animationFrames.size());

    // Frames are predecoded, so if rendering stalls we can jump directly to
    // the correct animation frame instead of executing a catch-up loop.
    g_basicWingAnimationFrame =
        static_cast<unsigned int>(
            (static_cast<std::uint64_t>(
                 g_basicWingAnimationFrame) +
             elapsedTicks) %
            frameCount);

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

static float wrapRadians(float a)
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTau = kPi * 2.0f;
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

static void updateTownFollowCamera()
{
    if (!g_townPlayerReady)
        return;

    const float deltaYaw =
        g_townPlayerYaw - g_townPlayerStartYaw;
    const float c = std::cos(deltaYaw);
    const float s = std::sin(deltaYaw);

    auto rotateOffsetY = [c, s](
        const float source[3],
        float out[3]) {
        out[0] = source[0] * c + source[2] * s;
        out[1] = source[1];
        out[2] = -source[0] * s + source[2] * c;
    };

    float cameraOffset[3]{};
    float targetOffset[3]{};
    rotateOffsetY(g_townCameraStartOffset, cameraOffset);
    rotateOffsetY(g_townTargetStartOffset, targetOffset);

    for (unsigned int i = 0; i < 3; ++i) {
        g_townCameraPosition[i] =
            g_townPlayerPosition[i] + cameraOffset[i];
        g_townCameraTarget[i] =
            g_townPlayerPosition[i] + targetOffset[i];
        g_townCameraUp[i] = g_townCameraPosition[i];
    }
    g_townCameraUp[1] += 1.0f;
}

static void resetTownPlayerRuntime()
{
    if (!g_townPlayerReady)
        return;

    std::memcpy(
        g_townPlayerPosition,
        g_townPlayerStartPosition,
        sizeof(g_townPlayerPosition));
    g_townPlayerYaw = g_townPlayerStartYaw;
    updateTownFollowCamera();
}

static void updateTownPlayerRuntime()
{
    if (!g_townPlayerReady)
        return;

    const float inputX = input::analog_x();
    const float inputForward = -input::analog_y();
    const float magnitude =
        std::min(
            1.0f,
            std::sqrt(
                inputX * inputX +
                inputForward * inputForward));

    if (magnitude <= 0.0001f)
        return;

    // Match town mode-1 semantics: stick direction is camera-relative, Edge
    // turns toward the requested heading with a bounded per-frame turn, and
    // forward speed is proportional to analog magnitude. Collision/ground
    // solving is deliberately the next milestone slice.
    float cameraForwardX =
        g_townCameraTarget[0] - g_townCameraPosition[0];
    float cameraForwardZ =
        g_townCameraTarget[2] - g_townCameraPosition[2];
    float forwardLength =
        std::sqrt(
            cameraForwardX * cameraForwardX +
            cameraForwardZ * cameraForwardZ);
    if (forwardLength <= 0.0001f) {
        cameraForwardX = std::sin(g_townPlayerYaw);
        cameraForwardZ = std::cos(g_townPlayerYaw);
        forwardLength = 1.0f;
    }
    cameraForwardX /= forwardLength;
    cameraForwardZ /= forwardLength;

    const float cameraRightX = cameraForwardZ;
    const float cameraRightZ = -cameraForwardX;

    float desiredX =
        cameraRightX * inputX +
        cameraForwardX * inputForward;
    float desiredZ =
        cameraRightZ * inputX +
        cameraForwardZ * inputForward;
    const float desiredLength =
        std::sqrt(desiredX * desiredX + desiredZ * desiredZ);
    if (desiredLength <= 0.0001f)
        return;
    desiredX /= desiredLength;
    desiredZ /= desiredLength;

    const float desiredYaw =
        std::atan2(desiredX, desiredZ);
    float yawDelta =
        wrapRadians(desiredYaw - g_townPlayerYaw);

    // Azel's town mode-1 minimum steering clamp is 0xE38E3 in its
    // 0x10000000-per-turn angle space.
    constexpr float kTau =
        6.28318530717958647692f;
    constexpr float kMaxTurnPerFrame =
        (static_cast<float>(0x0E38E3) /
         static_cast<float>(0x10000000)) * kTau;
    yawDelta =
        std::max(
            -kMaxTurnPerFrame,
            std::min(kMaxTurnPerFrame, yawDelta));
    g_townPlayerYaw =
        wrapRadians(g_townPlayerYaw + yawDelta);

    // Normal walk uses -0x109 16.16 units at full input in
    // updateEdgePositionSub1(). The sign there is local -Z; convert to our
    // world forward vector here.
    constexpr float kWalkStep =
        static_cast<float>(0x109) / 65536.0f;
    const float step = kWalkStep * magnitude;
    g_townPlayerPosition[0] +=
        std::sin(g_townPlayerYaw) * step;
    g_townPlayerPosition[2] +=
        std::cos(g_townPlayerYaw) * step;

    updateTownFollowCamera();
}

static ViewerMat4 buildAuthenticRoomWvp()
{
    const ViewerMat4 view =
        viewerLookAtLH(
            g_townPlayerReady
                ? g_townCameraPosition
                : g_staticRoomCpuMesh.cameraPosition,
            g_townPlayerReady
                ? g_townCameraTarget
                : g_staticRoomCpuMesh.cameraTarget,
            g_townPlayerReady
                ? g_townCameraUp
                : g_staticRoomCpuMesh.cameraUp);

    ViewerMat4 projection =
        buildAzelProjection(
            g_staticRoomCpuMesh.cameraFovDegrees,
            0u,
            g_staticRoomCpuMesh.cameraNear,
            g_staticRoomCpuMesh.cameraFar);

    // Saturn reference captures show our reconstructed town presentation is
    // horizontally reversed. Mirror only clip-space X here so camera-space
    // depth, Azel town visibility and LOD remain unchanged.
    projection.m[0] = -projection.m[0];

    return viewerMul(view, projection);
}

static void transformViewerPoint(
    const ViewerMat4& matrix,
    const float point[3],
    float out[3])
{
    out[0] =
        point[0] * matrix.m[0] +
        point[1] * matrix.m[4] +
        point[2] * matrix.m[8] +
        matrix.m[12];
    out[1] =
        point[0] * matrix.m[1] +
        point[1] * matrix.m[5] +
        point[2] * matrix.m[9] +
        matrix.m[13];
    out[2] =
        point[0] * matrix.m[2] +
        point[1] * matrix.m[6] +
        point[2] * matrix.m[10] +
        matrix.m[14];
}

static bool updateStaticRoomAzelTownVisibility()
{
    if (!g_staticRoomCpuReady ||
        !g_staticRoomCpuMesh.cameraValid)
        return true;

    const ViewerMat4 view =
        viewerLookAtLH(
            g_townPlayerReady
                ? g_townCameraPosition
                : g_staticRoomCpuMesh.cameraPosition,
            g_townPlayerReady
                ? g_townCameraTarget
                : g_staticRoomCpuMesh.cameraTarget,
            g_townPlayerReady
                ? g_townCameraUp
                : g_staticRoomCpuMesh.cameraUp);

    float cellCamera[3]{};
    transformViewerPoint(
        view,
        g_staticRoomCpuMesh.cellOrigin,
        cellCamera);

    constexpr float kPi =
        3.14159265358979323846f;
    const float halfFov =
        (g_staticRoomCpuMesh.cameraFovDegrees * 0.5f) *
        kPi / 180.0f;
    const float r0 =
        176.0f / std::tan(halfFov);
    const float widthScale =
        r0 * (352.0f / 320.0f);

    // initVDP1Projection() derives these exact Saturn fixed-point ratios:
    // m2C_widthRatio  = 176 / widthScale
    // m28_widthRatio2 = sqrt(176^2 + widthScale^2) / widthScale
    const float widthRatio =
        176.0f / widthScale;
    const float widthRatio2 =
        std::sqrt(
            176.0f * 176.0f +
            widthScale * widthScale) /
        widthScale;

    const float cellRadius =
        g_staticRoomCpuMesh.cellRadius;

    bool visible =
        cellCamera[2] >=
        g_staticRoomCpuMesh.cameraNear - cellRadius;

    if (visible) {
        const float horizontalLimit =
            cellCamera[2] * widthRatio +
            cellRadius * widthRatio2;
        visible =
            cellCamera[0] >= -horizontalLimit &&
            cellCamera[0] <= horizontalLimit;
    }

    g_staticRoomAzelCellVisible = visible;
    g_staticRoomAzelLod0Objects = 0;
    g_staticRoomAzelNonzeroLodObjects = 0;

    if (!visible)
        return false;

    // Mirrors:
    //   r5 = generateObjectMatrix(...)
    //   r4 = 0;
    //   while (r5 > gTownGrid.m3C[r4]) r4++;
    // For TWN_RUIN the only threshold is 0x7FFFFFFF, so every object should
    // resolve to LOD 0. Keep the actual walk here so later towns can replace
    // the threshold table without changing renderer semantics.
    for (const auto& object :
         g_staticRoomCpuMesh.objectStates) {
        float objectCamera[3]{};
        transformViewerPoint(
            view,
            object.worldOrigin,
            objectCamera);

        const std::int64_t depthFixed =
            static_cast<std::int64_t>(
                std::llround(
                    objectCamera[2] * 65536.0f));

        unsigned int lod = 0;
        while (lod + 1u <
                   g_staticRoomCpuMesh.lodDepthCount &&
               depthFixed >
                   g_staticRoomCpuMesh
                       .lodDepthThresholds[lod]) {
            ++lod;
        }

        if (lod == 0u)
            ++g_staticRoomAzelLod0Objects;
        else
            ++g_staticRoomAzelNonzeroLodObjects;
    }

    return true;
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

    // The room data is authored in Saturn/Azel screen handedness, which is
    // horizontally opposite to the GXM clip-space presentation used here.
    // Apply the same X presentation mirror as the authentic room camera so
    // both room viewers agree without changing the recovered world geometry.
    if (roomMode)
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
    // Native GXM Basic Wing viewer. The proven draw/scanout path is retained;
    // model rotation, camera placement, and perspective are now supplied
    // through the vertex shader's WVP uniform.
    if (g_gxmProbeAttempted) {
        if (!g_viewerReady)
            return;

        g_debugVisible = !g_debugVisible;
        g_probeDisplayingGxm = !g_debugVisible;
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
    rtParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
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

    if (sceGxmSyncObjectCreate(&g_probeSync2) < 0) {
        failure("[FAIL] GXM SYNC OBJECT 2");
        return;
    }

    status("[PASS] GXM COLOR SURFACES X2", 0xFF80E0FFu);

    const unsigned int alignedW =
        (kWidth + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1);
    const unsigned int alignedH =
        (kHeight + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1);
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
            SCE_GXM_MULTISAMPLE_NONE,
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

    const SceGxmProgram* textureVertexGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_texture_v_gxp_start);
    const SceGxmProgram* textureFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_texture_f_gxp_start);

    if (sceGxmProgramCheck(textureVertexGxp) < 0 ||
        sceGxmProgramCheck(textureFragmentGxp) < 0) {
        failure("[FAIL] TEXTURE GXP CHECK");
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
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            textureVertexGxp,
            &g_textureFragmentProgram) < 0) {
        failure("[FAIL] CREATE TEXTURE FP");
        return;
    }

    const SceGxmProgram* gouraudPayloadVertexGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_payload_v_gxp_start);
    const SceGxmProgram* gouraudDebugFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_debug_f_gxp_start);
    const SceGxmProgram* texturedLitFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_lit_f_gxp_start);
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
        sceGxmProgramCheck(gouraudDebugFragmentGxp) < 0 ||
        sceGxmProgramCheck(texturedLitFragmentGxp) < 0 ||
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_texturedLitFragmentProgram) < 0) {
        failure("[FAIL] CREATE TEXTURED LIT FP");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedLitNewtonFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
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
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            gouraudPayloadVertexGxp,
            &g_gouraudScanlineGrayFragmentProgram) < 0) {
        failure("[FAIL] CREATE SCANLINE GRAY FP");
        return;
    }

    status("[PASS] GXM GOURAUD VERTEX PAYLOAD", 0xFF80E0FFu);
    status("[PASS] GXM TEXTURED LIGHTING PIPELINE", 0xFF80E0FFu);
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

    // Stage 12: replace the proven triangle payload with the reconstructed
    // Basic Wing debug mesh. The GPU path remains otherwise unchanged.
    if (!g_basicWingCpuReady || g_basicWingCpuMesh.vertices.empty()) {
        failure("[FAIL] BASIC WING CPU MESH");
        return;
    }

    const Vdp1ModelSource vdp1Source = basicWingVdp1Source();
    if (!prepare_vdp1_model(vdp1Source)) {
        failure("[FAIL] BASIC WING VDP1 PREPARE");
        return;
    }
    g_residentVdp1Model = ResidentVdp1Model::BasicWing;
    status("[PASS] GXM BASIC WING VDP1 PREPARE", 0xFF80E0FFu);

    const unsigned int wingVertexCount =
        static_cast<unsigned int>(vdp1Source.vertexCount);

    const unsigned int wingAlignedW =
        (kWidth + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1);
    const unsigned int wingAlignedH =
        (kHeight + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1);

    std::memset(g_probeColorBuffer, 0,
                static_cast<std::size_t>(gxmPitch) * kHeight *
                sizeof(std::uint32_t));
    std::memset(g_probeDepth, 0xFF, wingAlignedW * wingAlignedH * 4u);
    std::memset(g_probeStencil, 0, wingAlignedW * wingAlignedH * 4u);

    const int wingBeginResult = sceGxmBeginScene(
        g_probeContext,
        0,
        g_probeRenderTarget,
        nullptr,
        nullptr,
        g_probeSync,
        &g_probeColorSurface,
        &g_probeDepthSurface);

    if (wingBeginResult < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] WING BEGIN 0X%08X",
                      static_cast<unsigned int>(wingBeginResult));
        failure(line);
        return;
    }

    sceGxmSetVertexProgram(g_probeContext, g_probeVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext, g_probeFragmentProgram);
    sceGxmSetCullMode(g_probeContext, SCE_GXM_CULL_NONE);
    sceGxmSetDefaultRegionClipAndViewport(
        g_probeContext, kWidth - 1, kHeight - 1);
    sceGxmSetFrontDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetBackDepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);

    void* wingUniformBuffer = nullptr;
    const int wingUniformResult =
        sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &wingUniformBuffer);
    if (wingUniformResult < 0 || !wingUniformBuffer) {
        sceGxmEndScene(g_probeContext, nullptr, nullptr);
        sceGxmFinish(g_probeContext);
        failure("[FAIL] WING WVP BUFFER");
        return;
    }

    const ViewerMat4 initialWvp = buildViewerWvp(false);
    sceGxmSetUniformDataF(
        wingUniformBuffer, g_probeWvpParam, 0, 16, initialWvp.m);

    const int wingStreamResult =
        sceGxmSetVertexStream(
            g_probeContext, 0, g_vdp1Vertices);
    if (wingStreamResult < 0) {
        sceGxmEndScene(g_probeContext, nullptr, nullptr);
        sceGxmFinish(g_probeContext);
        failure("[FAIL] WING VERTEX STREAM");
        return;
    }

    const int wingDrawResult = sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_vdp1Indices,
        wingVertexCount);

    if (wingDrawResult < 0) {
        sceGxmEndScene(g_probeContext, nullptr, nullptr);
        sceGxmFinish(g_probeContext);
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] WING DRAW 0X%08X",
                      static_cast<unsigned int>(wingDrawResult));
        failure(line);
        return;
    }

    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    sceGxmFinish(g_probeContext);

    status("[PASS] GXM BASIC WING DRAW", 0xFF80E0FFu);

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
        std::snprintf(line, sizeof(line), "[FAIL] GXM DISPLAY 0X%08X",
                      static_cast<unsigned int>(displayResult));
        failure(line);
        return;
    }

    sceDisplayWaitVblankStart();
    g_probeDisplayingGxm = true;
    g_gxmDrawBuffer = 1;
    g_debugVisible = false;
    g_suppressGxmInitPassStatus = false;
    status("[PASS] GXM INITIALIZATION + VDP1 READY", 0xFF80E0FFu);
    std::printf("[GXM] Basic Wing GXM color buffer queued for display\n");
}

bool debug_console_visible()
{
    return g_debugVisible;
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

    g_townPlayerReady =
        mesh.cameraValid &&
        mesh.edgeTransformValid;
    if (g_townPlayerReady) {
        std::memcpy(
            g_townPlayerPosition,
            mesh.edgePosition,
            sizeof(g_townPlayerPosition));
        std::memcpy(
            g_townPlayerStartPosition,
            mesh.edgePosition,
            sizeof(g_townPlayerStartPosition));

        // edgeRotation[] is stored in turns. Y is the town heading.
        constexpr float kTau =
            6.28318530717958647692f;
        g_townPlayerYaw =
            mesh.edgeRotation[1] * kTau;
        g_townPlayerStartYaw =
            g_townPlayerYaw;

        for (unsigned int i = 0; i < 3; ++i) {
            g_townCameraStartOffset[i] =
                mesh.cameraPosition[i] -
                mesh.edgePosition[i];
            g_townTargetStartOffset[i] =
                mesh.cameraTarget[i] -
                mesh.edgePosition[i];
        }
        updateTownFollowCamera();

        status(
            "[PASS] RUIN LIVE EDGE/FOLLOW STATE",
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
    g_basicWingAnimationLastUs = 0;
    g_basicWingAnimationPhase = 0;
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
    const bool gouraudPath =
        texturedLit || gouraudGray;

    sceGxmSetVertexProgram(
        g_probeContext,
        gouraudPath
            ? g_gouraudPayloadVertexProgram
            : (textured
                ? g_textureVertexProgram
                : g_probeVertexProgram));

    sceGxmSetFragmentProgram(
        g_probeContext,
        texturedLit
            ? g_texturedLitFragmentProgram
            : (gouraudGray
                ? g_gouraudDebugFragmentProgram
                : (textured
                    ? g_textureFragmentProgram
                    : g_probeFragmentProgram)));

    // Mirroring room projection X reverses triangle winding.
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
        gouraudPath
            ? g_gouraudPayloadWvpParam
            : (textured ? g_textureWvpParam : g_probeWvpParam),
        0, 16, drawState.wvp);

    const void* vertexStream =
        gouraudPath
            ? static_cast<const void*>(g_vdp1GouraudVertices)
            : (textured
                ? static_cast<const void*>(g_vdp1TextureVertices)
                : static_cast<const void*>(g_vdp1Vertices));

    if (sceGxmSetVertexStream(g_probeContext, 0, vertexStream) < 0)
        return false;

    if (textured) {
        for (unsigned int t = 0; t < g_vdp1GpuTextures.size(); ++t) {
            const TextureBatch& batch = g_vdp1TextureBatches[t];
            if (!batch.indexCount)
                continue;

            sceGxmSetFragmentTexture(
                g_probeContext, 0, &g_vdp1GpuTextures[t].texture);
            sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_vdp1TextureIndices + batch.firstIndex,
                batch.indexCount);
        }
        return true;
    }

    if (gouraudPath) {
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

        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            const std::uint16_t textureIndex =
                texturedLit ? model.polygonTextureIndices[p] : 0u;
            if (textureIndex >= textureBucketCount)
                continue;

            ViewerScreenPoint screen[4];
            bool visible = true;
            for (unsigned int corner = 0; corner < 4; ++corner) {
                screen[corner] = projectViewerPoint(
                    wvp,
                    model.vertices[p * 6u + cornerVertex[corner]]);
                if (!screen[corner].valid)
                    visible = false;
            }
            if (!visible)
                continue;

            visibleQuads[p] = 1u;

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
        }

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

        unsigned int submittedBatches = 0u;
        for (unsigned int t = 0; t < textureBucketCount; ++t) {
            const TextureBatch& batch = g_vdp1TextureBatches[t];
            if (!batch.indexCount)
                continue;

            if (texturedLit) {
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

        return submittedBatches != 0u ||
               totalVisibleIndices == 0u;
    }

    sceGxmDraw(
        g_probeContext,
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,
        g_vdp1Indices,
        static_cast<unsigned int>(model.vertexCount));
    return true;
}

static void renderBasicWingViewer()
{
    if (!g_viewerReady || !g_gxmInitialized || !g_probeContext ||
        !g_probeRenderTarget || !g_probeColorBuffer || !g_probeColorBuffer2 ||
        !g_probeVertexProgram || !g_probeFragmentProgram ||
        !g_vdp1Vertices || !g_vdp1LightingVertices ||
        !g_vdp1Indices)
        return;

    const int previousMode = g_viewMode;

    const int viewerModeCount =
        g_staticRoomCpuReady
            ? (g_staticRoomCpuMesh.cameraValid &&
               g_staticRoomCpuMesh.lightingValid
                ? 11
                : (g_staticRoomCpuMesh.lightingValid ? 7 : 6))
            : 5;
    if (input::prev_mode_pressed())
        g_viewMode = (g_viewMode + viewerModeCount - 1) % viewerModeCount;
    if (input::next_mode_pressed())
        g_viewMode = (g_viewMode + 1) % viewerModeCount;
    if (input::resolution_toggle_pressed())
        g_halfResolution = !g_halfResolution;

    const bool roomMode =
        g_staticRoomCpuReady && g_viewMode >= 5;
    const bool previousRoomMode =
        g_staticRoomCpuReady && previousMode >= 5;
    if (roomMode != previousRoomMode) {
        g_viewDistance =
            roomMode ? g_staticRoomFitDistance : g_basicWingFitDistance;
    }

    const bool roomAuthenticCameraMode =
        g_staticRoomCpuReady &&
        g_staticRoomCpuMesh.cameraValid &&
        g_viewMode >= 7;

    // Debug orbit controls alter only the view transform. Authentic modes use
    // the recovered Azel startup camera.
    if (!roomAuthenticCameraMode) {
        g_viewYaw += input::analog_x() * 0.035f;
        g_viewPitch += input::analog_y() * 0.035f;
        g_viewPitch =
            std::max(-1.45f, std::min(1.45f, g_viewPitch));

        const float fitDistance =
            roomMode ? g_staticRoomFitDistance : g_basicWingFitDistance;
        const float dollyStep =
            std::max(0.0025f, fitDistance * 0.02f);

        g_viewDistance += input::analog_zoom() * dollyStep;
        g_viewDistance =
            std::max(0.01f, std::min(32.0f, g_viewDistance));
    }

    if (input::reset_view_pressed()) {
        if (roomAuthenticCameraMode) {
            resetTownPlayerRuntime();
        } else {
            g_viewYaw = 0.60f;
            g_viewPitch = -0.30f;
            g_viewDistance =
                roomMode ? g_staticRoomFitDistance : g_basicWingFitDistance;
        }
    }

    if (roomAuthenticCameraMode)
        updateTownPlayerRuntime();

    const bool roomDiagnosticLitMode =
        g_staticRoomCpuReady &&
        g_staticRoomCpuMesh.lightingValid &&
        g_viewMode == 6;
    const bool roomAuthenticLitMode =
        roomAuthenticCameraMode &&
        g_viewMode == 7;
    const bool roomAuthenticTexturedMode =
        roomAuthenticCameraMode &&
        g_viewMode == 8;
    const bool roomAuthenticFlatMode =
        roomAuthenticCameraMode &&
        g_viewMode == 9;
    const bool roomAuthenticLightingOnlyMode =
        roomAuthenticCameraMode &&
        g_viewMode == 10;

    if (!roomMode && g_residentVdp1Model != ResidentVdp1Model::BasicWing) {
        if (!prepare_vdp1_model(basicWingVdp1Source()))
            return;
        g_residentVdp1Model = ResidentVdp1Model::BasicWing;
        applyBasicWingAnimationFrame(g_basicWingAnimationFrame);
    } else if (roomMode) {
        const ResidentVdp1Model desiredResident =
            roomAuthenticCameraMode
                ? ResidentVdp1Model::StaticRoomAuthentic
                : ResidentVdp1Model::StaticRoomDiagnostic;

        if (g_residentVdp1Model != desiredResident) {
            if (!prepare_vdp1_model(
                    staticRoomVdp1Source(roomAuthenticCameraMode)))
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

    bool azelTownCellVisible = true;
    if (roomAuthenticCameraMode)
        azelTownCellVisible = updateStaticRoomAzelTownVisibility();

    if ((roomDiagnosticLitMode ||
         roomAuthenticLitMode ||
         roomAuthenticLightingOnlyMode) &&
        azelTownCellVisible) {
        updateStaticRoomAzelLighting(roomAuthenticCameraMode);
    }

    Vdp1DrawState drawState{};
    std::memcpy(drawState.wvp, wvp.m, sizeof(drawState.wvp));
    drawState.mode = renderMode;
    drawState.reverseCullWinding = roomMode;

    const Vdp1ModelSource model =
        roomMode
            ? staticRoomVdp1Source(roomAuthenticCameraMode)
            : basicWingVdp1Source();

    if (azelTownCellVisible) {
        if (!submit_vdp1_model(model, drawState)) {
            sceGxmEndScene(g_probeContext, nullptr, nullptr);
            sceGxmFinish(g_probeContext);
            return;
        }
    }

    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    sceGxmFinish(g_probeContext);

    drawViewerModeOverlay(
        colorBuffer,
        gxmPitch,
        g_viewMode);

    drawTextSmallToBuffer(
        colorBuffer,
        gxmPitch,
        16,
        viewerRenderHeight() - 12,
        g_halfResolution ? "480X272 GXM" : "960X544 NATIVE",
        0xFFFFFFFFu);

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = colorBuffer;
    fb.pitch = gxmPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = viewerRenderWidth();
    fb.height = viewerRenderHeight();

    waitFor30HzPresentSlot();
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    mark30HzPresented();

    // The buffer just queued is now front; draw the next frame into the
    // opposite GXM surface so scanout and rendering never touch the same
    // memory concurrently.
    g_gxmDrawBuffer ^= 1;
}

void begin_frame()
{
    if (!g_debugVisible) {
        renderBasicWingViewer();
        return;
    }

    fill(0xFF181818u);

    drawText(32, 24, "LAGI - PDS VITA RUNTIME", 0xFFFFFFFFu, 2);
    drawTextSmall(32, 48, "BOOT / INTEGRATION STATUS", 0xFFB0B0B0u);

    for (int i = 0; i < g_statusCount; ++i) {
        const int column = i / kStatusRowsPerColumn;
        const int row = i % kStatusRowsPerColumn;
        if (column >= 2)
            break;

        drawTextSmall(
            kStatusColumnX[column],
            68 + row * kStatusLineHeight,
            g_status[i].text,
            g_status[i].color);
    }

    if (g_azelAlive) {
        const int taskIndex = std::min(g_statusCount, kMaxStatus - 1);
        const int column = std::min(taskIndex / kStatusRowsPerColumn, 1);
        const int row = taskIndex % kStatusRowsPerColumn;
        drawTextSmall(
            kStatusColumnX[column],
            72 + row * kStatusLineHeight,
            "TASK LOOP: ACTIVE",
            0xFF30E030u);
    }
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

} // namespace lagi::platform::renderer
