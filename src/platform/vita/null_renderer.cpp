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
extern const unsigned char _binary_lagi_gouraud_debug_f_gxp_start[];
extern const unsigned char _binary_lagi_textured_lit_f_gxp_start[];
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
static SceUID g_probeColorUid = -1;
static std::uint32_t* g_probeColorBuffer = nullptr;
static SceGxmColorSurface g_probeColorSurface{};
static SceGxmSyncObject* g_probeSync = nullptr;
static SceUID g_probeColorUid2 = -1;
static std::uint32_t* g_probeColorBuffer2 = nullptr;
static SceGxmColorSurface g_probeColorSurface2{};
static SceGxmSyncObject* g_probeSync2 = nullptr;
static int g_gxmDrawBuffer = 1;
static SceUID g_probeDepthUid = -1;
static SceUID g_probeStencilUid = -1;
static void* g_probeDepth = nullptr;
static void* g_probeStencil = nullptr;
static SceGxmDepthStencilSurface g_probeDepthSurface{};
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
static SceGxmShaderPatcherId g_gouraudDebugFragmentProgramId{};
static bool g_gouraudDebugFragmentRegistered = false;
static SceGxmFragmentProgram* g_gouraudDebugFragmentProgram = nullptr;
static const SceGxmProgramParameter* g_gouraudDebugQuadScreen01Param = nullptr;
static const SceGxmProgramParameter* g_gouraudDebugQuadScreen23Param = nullptr;
static const SceGxmProgramParameter* g_gouraudDebugGouraudRParam = nullptr;
static const SceGxmProgramParameter* g_gouraudDebugGouraudGParam = nullptr;
static const SceGxmProgramParameter* g_gouraudDebugGouraudBParam = nullptr;

static SceGxmShaderPatcherId g_texturedLitFragmentProgramId{};
static bool g_texturedLitFragmentRegistered = false;
static SceGxmFragmentProgram* g_texturedLitFragmentProgram = nullptr;

static const SceGxmProgramParameter* g_textureWvpParam = nullptr;
static const SceGxmProgramParameter* g_texturedLitQuadScreen01Param = nullptr;
static const SceGxmProgramParameter* g_texturedLitQuadScreen23Param = nullptr;
static const SceGxmProgramParameter* g_texturedLitGouraudRParam = nullptr;
static const SceGxmProgramParameter* g_texturedLitGouraudGParam = nullptr;
static const SceGxmProgramParameter* g_texturedLitGouraudBParam = nullptr;
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
static azel::DebugTextureVertex* g_vdp1GouraudVertices = nullptr;
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
static int g_viewMode = 0;
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
    static constexpr int srcW = 5;
    static constexpr int srcH = 7;
    static constexpr int dstW = 7;  // ceil(5 * 4/3)
    static constexpr int dstH = 10; // ceil(7 * 4/3)

    for (int dy = 0; dy < dstH; ++dy) {
        const int gy = (dy * 3) / 4;
        const int py = y + dy;
        if (py < 0 || py >= kHeight)
            continue;

        for (int dx = 0; dx < dstW; ++dx) {
            const int gx = (dx * 3) / 4;
            if (gx >= srcW || gy >= srcH)
                continue;
            if (!(rows[gy] & (1u << (4 - gx))))
                continue;

            const int px = x + dx;
            if (px >= 0 && px < kWidth)
                g_frameBuffer[g_drawBuffer][py * kPitch + px] = color;
        }
    }
}

static void drawTextSmall(int x, int y, const char* text, std::uint32_t color)
{
    if (!text) return;
    static constexpr int advance = 8; // 6 * 4/3

    for (const char* p = text; *p; ++p) {
        drawCharSmall(x, y, *p, color);
        x += advance;
        if (x > kWidth - advance)
            break;
    }
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
        if (g_textureVertexProgram) {
            sceGxmShaderPatcherReleaseVertexProgram(
                g_probeShaderPatcher, g_textureVertexProgram);
            g_textureVertexProgram = nullptr;
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

    const unsigned int vertexBytes =
        vertexCount * sizeof(azel::DebugTextureVertex);
    const unsigned int indexBytes =
        vertexCount * sizeof(std::uint16_t);

    g_vdp1TextureVertices =
        static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                vertexBytes,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &g_vdp1TextureVertexUid));
    g_vdp1GouraudVertices =
        static_cast<azel::DebugTextureVertex*>(
            probeGpuAlloc(
                vertexBytes,
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

            const auto& lightingSource =
                model.lightingVertices[vertexIndex];
            const float shade =
                static_cast<float>(lightingSource.r) / 255.0f;
            g_vdp1GouraudVertices[vertexIndex] = {
                source.x, source.y, source.z,
                shade, 0.0f
            };
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

static ViewerMat4 viewerPerspective(
    float fovYRadians, float aspect, float nearZ, float farZ)
{
    // Row-vector, left-handed perspective for shader:
    //     mul(float4(position, 1), wvp)
    // Maps positive view-space Z into the normalized depth interval.
    const float yScale = 1.0f / std::tan(fovYRadians * 0.5f);
    const float xScale = yScale / aspect;
    const float zScale = farZ / (farZ - nearZ);

    ViewerMat4 r{};
    r.m[0] = xScale;
    r.m[5] = yScale;
    r.m[10] = zScale;
    r.m[11] = 1.0f;
    r.m[14] = -nearZ * zScale;
    return r;
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

static ViewerMat4 buildAuthenticRoomWvp()
{
    constexpr float kPi = 3.14159265358979323846f;

    // Match Azel initVDP1Projection(DEG_80 / 2, 0):
    //   r0          = 176 * cot(40 degrees)
    //   widthScale  = r0 * (352 / 320)
    //   heightScale = r0 * (224 / 240)
    // and convert the Saturn pixel projection to normalized coordinates.
    const float halfFov =
        (g_staticRoomCpuMesh.cameraFovDegrees * 0.5f) *
        kPi / 180.0f;
    const float cotHalf = 1.0f / std::tan(halfFov);
    const float saturnXScale =
        cotHalf * (352.0f / 320.0f);
    const float saturnYScale =
        cotHalf * (176.0f / 112.0f) *
        (224.0f / 240.0f);

    // Saturn's 352x224 image is intended for a 4:3 display. Scale X into
    // the central 4:3 region of Vita's 16:9 framebuffer so geometry keeps
    // the intended physical proportions. A later viewport/scissor pass can
    // make the original horizontal clipping boundary exact as well.
    const float intendedAspect = 4.0f / 3.0f;
    const float vitaAspect =
        static_cast<float>(kWidth) / static_cast<float>(kHeight);
    const float xDisplayCorrection =
        intendedAspect / vitaAspect;

    const float nearZ =
        g_staticRoomCpuMesh.cameraNear;
    const float farZ =
        g_staticRoomCpuMesh.cameraFar;
    const float zScale =
        farZ / (farZ - nearZ);

    ViewerMat4 projection{};
    projection.m[0] =
        saturnXScale * xDisplayCorrection;
    projection.m[5] = saturnYScale;
    projection.m[10] = zScale;
    projection.m[11] = 1.0f;
    projection.m[14] = -nearZ * zScale;

    const ViewerMat4 view =
        viewerLookAtLH(
            g_staticRoomCpuMesh.cameraPosition,
            g_staticRoomCpuMesh.cameraTarget,
            g_staticRoomCpuMesh.cameraUp);

    return viewerMul(view, projection);
}

static ViewerMat4 buildViewerWvp()
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kFovY = 50.0f * kPi / 180.0f;
    constexpr float kAspect =
        static_cast<float>(kWidth) / static_cast<float>(kHeight);
    constexpr float kNearZ = 0.10f;
    constexpr float kFarZ = 100.0f;
    const float cameraDistance = g_viewDistance;

    const ViewerMat4 yaw = viewerRotationY(g_viewYaw);
    const ViewerMat4 pitch = viewerRotationX(g_viewPitch);
    const ViewerMat4 view =
        viewerTranslation(0.0f, 0.0f, cameraDistance);
    const ViewerMat4 projection =
        viewerPerspective(kFovY, kAspect, kNearZ, kFarZ);

    // Shader uses a row vector, so transforms apply left-to-right.
    return viewerMul(
        viewerMul(viewerMul(yaw, pitch), view),
        projection);
}

struct ViewerScreenPoint
{
    float x = 0.0f;
    float y = 0.0f;
    bool valid = false;
};

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
    out.x = (ndcX * 0.5f + 0.5f) * static_cast<float>(kWidth);
    out.y = (0.5f - ndcY * 0.5f) * static_cast<float>(kHeight);
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
        // Row-vector yaw/pitch, then the viewer's +Z camera translation.
        const float z =
            v.x * rotation.m[2] +
            v.y * rotation.m[6] +
            v.z * rotation.m[10] +
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

            const int falloffIndex =
                falloffIndexForQuad(p);
            int accum[3] = {
                falloffMap[falloffIndex][0],
                falloffMap[falloffIndex][1],
                falloffMap[falloffIndex][2]
            };
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

            int accum[3] = {fallR, fallG, fallB};

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

    status("[PASS] GXM DEPTH SURFACE", 0xFF80E0FFu);

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

    const SceGxmProgram* gouraudDebugFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_gouraud_debug_f_gxp_start);
    if (sceGxmProgramCheck(gouraudDebugFragmentGxp) < 0 ||
        sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            gouraudDebugFragmentGxp,
            &g_gouraudDebugFragmentProgramId) < 0) {
        failure("[FAIL] GOURAUD DEBUG FP REG");
        return;
    }
    g_gouraudDebugFragmentRegistered = true;

    g_gouraudDebugQuadScreen01Param =
        sceGxmProgramFindParameterByName(
            gouraudDebugFragmentGxp, "quadScreen01");
    g_gouraudDebugQuadScreen23Param =
        sceGxmProgramFindParameterByName(
            gouraudDebugFragmentGxp, "quadScreen23");
    g_gouraudDebugGouraudRParam =
        sceGxmProgramFindParameterByName(
            gouraudDebugFragmentGxp, "gouraudR");
    g_gouraudDebugGouraudGParam =
        sceGxmProgramFindParameterByName(
            gouraudDebugFragmentGxp, "gouraudG");
    g_gouraudDebugGouraudBParam =
        sceGxmProgramFindParameterByName(
            gouraudDebugFragmentGxp, "gouraudB");

    if (!g_gouraudDebugQuadScreen01Param ||
        !g_gouraudDebugQuadScreen23Param ||
        !g_gouraudDebugGouraudRParam ||
        !g_gouraudDebugGouraudGParam ||
        !g_gouraudDebugGouraudBParam) {
        failure("[FAIL] GOURAUD DEBUG FP PARAMS");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_gouraudDebugFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            textureVertexGxp,
            &g_gouraudDebugFragmentProgram) < 0) {
        failure("[FAIL] CREATE GOURAUD DEBUG FP");
        return;
    }

    const SceGxmProgram* texturedLitFragmentGxp =
        reinterpret_cast<const SceGxmProgram*>(
            _binary_lagi_textured_lit_f_gxp_start);

    if (sceGxmProgramCheck(texturedLitFragmentGxp) < 0) {
        failure("[FAIL] TEXTURED LIT GXP CHECK");
        return;
    }

    if (sceGxmShaderPatcherRegisterProgram(
            g_probeShaderPatcher,
            texturedLitFragmentGxp,
            &g_texturedLitFragmentProgramId) < 0) {
        failure("[FAIL] TEXTURED LIT PROGRAM REG");
        return;
    }
    g_texturedLitFragmentRegistered = true;

    g_texturedLitQuadScreen01Param =
        sceGxmProgramFindParameterByName(
            texturedLitFragmentGxp, "quadScreen01");
    g_texturedLitQuadScreen23Param =
        sceGxmProgramFindParameterByName(
            texturedLitFragmentGxp, "quadScreen23");
    g_texturedLitGouraudRParam =
        sceGxmProgramFindParameterByName(
            texturedLitFragmentGxp, "gouraudR");
    g_texturedLitGouraudGParam =
        sceGxmProgramFindParameterByName(
            texturedLitFragmentGxp, "gouraudG");
    g_texturedLitGouraudBParam =
        sceGxmProgramFindParameterByName(
            texturedLitFragmentGxp, "gouraudB");

    if (!g_texturedLitQuadScreen01Param ||
        !g_texturedLitQuadScreen23Param ||
        !g_texturedLitGouraudRParam ||
        !g_texturedLitGouraudGParam ||
        !g_texturedLitGouraudBParam) {
        failure("[FAIL] TEXTURED LIT FP PARAMS");
        return;
    }

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_probeShaderPatcher,
            g_texturedLitFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            textureVertexGxp,
            &g_texturedLitFragmentProgram) < 0) {
        failure("[FAIL] CREATE TEXTURED LIT FP");
        return;
    }

    status("[PASS] GXM TEXTURED LIGHTING PIPELINE", 0xFF80E0FFu);

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

    const ViewerMat4 initialWvp = buildViewerWvp();
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

    float minX = g_basicWingCpuMesh.vertices[0].x;
    float maxX = minX;
    float minY = g_basicWingCpuMesh.vertices[0].y;
    float maxY = minY;
    float minZ = g_basicWingCpuMesh.vertices[0].z;
    float maxZ = minZ;

    for (const auto& v : g_basicWingCpuMesh.vertices) {
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
    }

    const float cx = (minX + maxX) * 0.5f;
    const float cy = (minY + maxY) * 0.5f;
    const float cz = (minZ + maxZ) * 0.5f;
    const float extentX = std::max(maxX - minX, 0.001f);
    const float extentY = std::max(maxY - minY, 0.001f);
    const float extentZ = std::max(maxZ - minZ, 0.001f);
    const float maxExtent = std::max(extentX, std::max(extentY, extentZ));
    const float scale = 1.45f / maxExtent;

    for (auto& v : g_basicWingCpuMesh.vertices) {
        v.x = (v.x - cx) * scale;
        v.y = (v.y - cy) * scale;
        v.z = (v.z - cz) * scale;
    }

    for (auto& frame : g_basicWingCpuMesh.animationFrames) {
        for (auto& v : frame.vertices) {
            v.x = (v.x - cx) * scale;
            v.y = (v.y - cy) * scale;
            v.z = (v.z - cz) * scale;
        }
    }

    if (g_basicWingCpuMesh.polygonRecords.size() !=
        g_basicWingCpuMesh.polygons) {
        g_basicWingCpuReady = false;
        g_viewerReady = false;
        return false;
    }

    g_viewYaw = 0.60f;
    g_viewPitch = -0.30f;
    g_viewDistance = 3.0f;
    g_viewMode = 0;
    g_basicWingAnimationFrame = 0;
    g_basicWingAnimationLastUs = 0;
    g_basicWingAnimationPhase = 0;
    g_presentClockInitialized = false;
    g_lastPresentVcount = 0;
    g_basicWingCpuReady = true;
    g_viewerReady = true;
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
        g_vdp1TextureVertices && g_vdp1TextureIndices;
    const bool gouraudDebug =
        drawState.mode == Vdp1RenderMode::GouraudGrayscale &&
        g_vdp1TexturedReady && model.texturesValid() &&
        g_vdp1TextureVertices;
    const bool wireframe =
        drawState.mode == Vdp1RenderMode::Wireframe;

    sceGxmSetVertexProgram(
        g_probeContext,
        (textured || texturedLit || gouraudDebug)
            ? g_textureVertexProgram
            : g_probeVertexProgram);
    sceGxmSetFragmentProgram(
        g_probeContext,
        texturedLit
            ? g_texturedLitFragmentProgram
            : (textured
                ? g_textureFragmentProgram
                : (gouraudDebug
                    ? g_gouraudDebugFragmentProgram
                    : g_probeFragmentProgram)));

    sceGxmSetCullMode(
        g_probeContext,
        wireframe ? SCE_GXM_CULL_NONE : SCE_GXM_CULL_CW);
    sceGxmSetDefaultRegionClipAndViewport(
        g_probeContext, kWidth - 1, kHeight - 1);

    const SceGxmDepthFunc depthFunc =
        wireframe ? SCE_GXM_DEPTH_FUNC_LESS : SCE_GXM_DEPTH_FUNC_LESS_EQUAL;
    sceGxmSetFrontDepthFunc(g_probeContext, depthFunc);
    sceGxmSetBackDepthFunc(g_probeContext, depthFunc);
    sceGxmSetFrontDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(
        g_probeContext, SCE_GXM_DEPTH_WRITE_ENABLED);

    const SceGxmPolygonMode polygonMode =
        wireframe ? SCE_GXM_POLYGON_MODE_LINE : SCE_GXM_POLYGON_MODE_TRIANGLE_FILL;
    sceGxmSetFrontPolygonMode(g_probeContext, polygonMode);
    sceGxmSetBackPolygonMode(g_probeContext, polygonMode);

    void* uniformBuffer = nullptr;
    if (sceGxmReserveVertexDefaultUniformBuffer(
            g_probeContext, &uniformBuffer) < 0 || !uniformBuffer)
        return false;

    sceGxmSetUniformDataF(
        uniformBuffer,
        (textured || texturedLit || gouraudDebug)
            ? g_textureWvpParam
            : g_probeWvpParam,
        0, 16, drawState.wvp);

    const void* vertexStream =
        (textured || texturedLit || gouraudDebug)
            ? static_cast<const void*>(g_vdp1TextureVertices)
            : static_cast<const void*>(g_vdp1Vertices);

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

    if (texturedLit || gouraudDebug) {
        // Preserve original Saturn quad identity. The fragment shaders receive
        // all four projected corners and all four RGB555 Gouraud values, then
        // perform the proven inverse-bilinear reconstruction per pixel.
        static const unsigned int cornerVertex[4] = {0, 1, 2, 5};
        ViewerMat4 wvp{};
        std::memcpy(wvp.m, drawState.wvp, sizeof(wvp.m));

        for (unsigned int p = 0;
             p < static_cast<unsigned int>(model.polygonCount); ++p) {
            const std::uint16_t textureIndex = model.polygonTextureIndices[p];
            if (texturedLit && textureIndex >= g_vdp1GpuTextures.size())
                continue;

            ViewerScreenPoint screen[4];
            bool visible = true;
            for (unsigned int corner = 0; corner < 4; ++corner) {
                screen[corner] = projectViewerPoint(
                    wvp, model.vertices[p * 6u + cornerVertex[corner]]);
                if (!screen[corner].valid)
                    visible = false;
            }
            if (!visible)
                continue;

            const float quadScreen01[4] = {
                screen[0].x, screen[0].y,
                screen[1].x, screen[1].y
            };
            const float quadScreen23[4] = {
                screen[2].x, screen[2].y,
                screen[3].x, screen[3].y
            };

            const auto& gouraud = model.gouraud555[p];
            const float gouraudR[4] = {
                gouraud.corner[0][0], gouraud.corner[1][0],
                gouraud.corner[2][0], gouraud.corner[3][0]
            };
            const float gouraudG[4] = {
                gouraud.corner[0][1], gouraud.corner[1][1],
                gouraud.corner[2][1], gouraud.corner[3][1]
            };
            const float gouraudB[4] = {
                gouraud.corner[0][2], gouraud.corner[1][2],
                gouraud.corner[2][2], gouraud.corner[3][2]
            };

            void* fragmentUniformBuffer = nullptr;
            if (sceGxmReserveFragmentDefaultUniformBuffer(
                    g_probeContext, &fragmentUniformBuffer) < 0 ||
                !fragmentUniformBuffer)
                continue;

            const SceGxmProgramParameter* quad01Param =
                texturedLit ? g_texturedLitQuadScreen01Param
                            : g_gouraudDebugQuadScreen01Param;
            const SceGxmProgramParameter* quad23Param =
                texturedLit ? g_texturedLitQuadScreen23Param
                            : g_gouraudDebugQuadScreen23Param;
            const SceGxmProgramParameter* rParam =
                texturedLit ? g_texturedLitGouraudRParam
                            : g_gouraudDebugGouraudRParam;
            const SceGxmProgramParameter* gParam =
                texturedLit ? g_texturedLitGouraudGParam
                            : g_gouraudDebugGouraudGParam;
            const SceGxmProgramParameter* bParam =
                texturedLit ? g_texturedLitGouraudBParam
                            : g_gouraudDebugGouraudBParam;

            sceGxmSetUniformDataF(fragmentUniformBuffer, quad01Param, 0, 4, quadScreen01);
            sceGxmSetUniformDataF(fragmentUniformBuffer, quad23Param, 0, 4, quadScreen23);
            sceGxmSetUniformDataF(fragmentUniformBuffer, rParam, 0, 4, gouraudR);
            sceGxmSetUniformDataF(fragmentUniformBuffer, gParam, 0, 4, gouraudG);
            sceGxmSetUniformDataF(fragmentUniformBuffer, bParam, 0, 4, gouraudB);

            if (texturedLit) {
                sceGxmSetFragmentTexture(
                    g_probeContext, 0,
                    &g_vdp1GpuTextures[textureIndex].texture);
            }

            sceGxmDraw(
                g_probeContext,
                SCE_GXM_PRIMITIVE_TRIANGLES,
                SCE_GXM_INDEX_FORMAT_U16,
                g_vdp1Indices + p * 6u,
                6);
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

static void renderBasicWingViewer()
{
    if (!g_viewerReady || !g_gxmInitialized || !g_probeContext ||
        !g_probeRenderTarget || !g_probeColorBuffer || !g_probeColorBuffer2 ||
        !g_probeVertexProgram || !g_probeFragmentProgram ||
        !g_vdp1Vertices || !g_vdp1LightingVertices ||
        !g_vdp1Indices)
        return;

    g_viewYaw += input::analog_x() * 0.035f;
    g_viewPitch += input::analog_y() * 0.035f;
    g_viewPitch = std::max(-1.45f, std::min(1.45f, g_viewPitch));

    // Right stick Y dollies the camera without altering FOV.
    // Pushing up moves closer; pulling down moves farther away.
    g_viewDistance += input::analog_zoom() * 0.060f;
    g_viewDistance = std::max(0.75f, std::min(8.0f, g_viewDistance));

    if (input::reset_view_pressed()) {
        g_viewYaw = 0.60f;
        g_viewPitch = -0.30f;
        g_viewDistance = 3.0f;
    }

    const int viewerModeCount =
        g_staticRoomCpuReady
            ? (g_staticRoomCpuMesh.cameraValid &&
               g_staticRoomCpuMesh.lightingValid
                ? 8
                : (g_staticRoomCpuMesh.lightingValid ? 7 : 6))
            : 5;
    if (input::prev_mode_pressed())
        g_viewMode = (g_viewMode + viewerModeCount - 1) % viewerModeCount;
    if (input::next_mode_pressed())
        g_viewMode = (g_viewMode + 1) % viewerModeCount;

    const bool roomMode =
        g_staticRoomCpuReady && g_viewMode >= 5;
    const bool roomLitMode =
        g_staticRoomCpuReady &&
        g_staticRoomCpuMesh.lightingValid &&
        g_viewMode >= 6;
    const bool roomAuthenticCameraMode =
        g_staticRoomCpuReady &&
        g_staticRoomCpuMesh.cameraValid &&
        g_viewMode == 7;

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
                    staticRoomVdp1Source(
                        roomAuthenticCameraMode)))
                return;
            g_residentVdp1Model = desiredResident;
        }
    }

    if (!roomMode && g_basicWingCpuMesh.animationValid)
        advanceBasicWingAnimation();

    const ViewerMat4 wvp =
        roomAuthenticCameraMode
            ? buildAuthenticRoomWvp()
            : buildViewerWvp();

    constexpr int gxmPitch = 1024;

    std::uint32_t* const colorBuffer =
        g_gxmDrawBuffer == 0 ? g_probeColorBuffer : g_probeColorBuffer2;
    SceGxmColorSurface* const colorSurface =
        g_gxmDrawBuffer == 0 ? &g_probeColorSurface : &g_probeColorSurface2;
    SceGxmSyncObject* const syncObject =
        g_gxmDrawBuffer == 0 ? g_probeSync : g_probeSync2;

    const unsigned int alignedW =
        (kWidth + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1);
    const unsigned int alignedH =
        (kHeight + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1);

    std::memset(colorBuffer, 0,
                static_cast<std::size_t>(gxmPitch) * kHeight *
                sizeof(std::uint32_t));
    std::memset(g_probeDepth, 0xFF, alignedW * alignedH * 4u);
    std::memset(g_probeStencil, 0, alignedW * alignedH * 4u);

    if (sceGxmBeginScene(
            g_probeContext, 0, g_probeRenderTarget,
            nullptr, nullptr, syncObject,
            colorSurface, &g_probeDepthSurface) < 0)
        return;

    const Vdp1RenderMode renderMode =
        roomMode
            ? (g_staticRoomCpuMesh.texturesFullyResolved
                ? (roomLitMode
                    ? Vdp1RenderMode::TexturedGouraud
                    : Vdp1RenderMode::Textured)
                : Vdp1RenderMode::PolygonColor)
            : static_cast<Vdp1RenderMode>(g_viewMode);

    if (!roomMode &&
        (renderMode == Vdp1RenderMode::TexturedGouraud ||
         renderMode == Vdp1RenderMode::GouraudGrayscale))
        updateViewerAzelLighting();

    if (roomLitMode)
        updateStaticRoomAzelLighting(
            roomAuthenticCameraMode);

    Vdp1DrawState drawState{};
    std::memcpy(drawState.wvp, wvp.m, sizeof(drawState.wvp));
    drawState.mode = renderMode;

    const Vdp1ModelSource model =
        roomMode
            ? staticRoomVdp1Source(roomAuthenticCameraMode)
            : basicWingVdp1Source();
    if (!submit_vdp1_model(model, drawState)) {
        sceGxmEndScene(g_probeContext, nullptr, nullptr);
        sceGxmFinish(g_probeContext);
        return;
    }

    sceGxmEndScene(g_probeContext, nullptr, nullptr);
    sceGxmFinish(g_probeContext);

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = colorBuffer;
    fb.pitch = gxmPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

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
