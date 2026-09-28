#include "lagi/platform.h"
#include "lagi/debug_mesh.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>

namespace lagi::platform::renderer {

extern "C" {
extern const unsigned char lagi_color_v_gxp[];
extern const unsigned char lagi_color_f_gxp[];
}

static constexpr int kWidth = 960;
static constexpr int kHeight = 544;
static constexpr int kPitch = 960;
static constexpr std::size_t kFrameBytes =
    static_cast<std::size_t>(kPitch) * kHeight * sizeof(std::uint32_t);
static constexpr int kMaxStatus = 64;

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
static bool g_probeScenePassed = false;
static azel::BasicWingDebugMesh g_basicWingCpuMesh{};
static bool g_basicWingCpuReady = false;
static SceUID g_basicWingVertexUid = -1;
static SceUID g_basicWingIndexUid = -1;
static azel::DebugColorVertex* g_basicWingVertices = nullptr;
static std::uint16_t* g_basicWingIndices = nullptr;
static bool g_probeDisplayingGxm = false;

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

    void* basicWingVertexPtr = g_basicWingVertices;
    freeSimpleMappedProbe(g_basicWingVertexUid, basicWingVertexPtr);
    g_basicWingVertices = nullptr;
    void* basicWingIndexPtr = g_basicWingIndices;
    freeSimpleMappedProbe(g_basicWingIndexUid, basicWingIndexPtr);
    g_basicWingIndices = nullptr;

    if (g_probeShaderPatcher) {
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
}

void status(const char* text, unsigned int color)
{
    if (!text || g_statusCount >= kMaxStatus) return;
    StatusLine& line = g_status[g_statusCount++];
    std::strncpy(line.text, text, sizeof(line.text) - 1);
    line.text[sizeof(line.text) - 1] = 0;
    line.color = color;
}

void failure(const char* text)
{
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

void toggle_debug_console()
{
    // Stage 12 native GXM probe: use the fully proven GXM pipeline to draw the
    // reconstructed Basic Wing mesh with fixed CPU-side presentation view
    // and per-polygon debug colors, then queue it for display.
    g_debugVisible = true;

    if (g_gxmProbeAttempted)
        return;

    g_gxmProbeAttempted = true;

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

    status("[PASS] GXM COLOR SURFACE", 0xFF80E0FFu);

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
        reinterpret_cast<const SceGxmProgram*>(lagi_color_v_gxp);
    const SceGxmProgram* fragmentProgram =
        reinterpret_cast<const SceGxmProgram*>(lagi_color_f_gxp);

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

    const unsigned int wingVertexCount =
        static_cast<unsigned int>(g_basicWingCpuMesh.vertices.size());
    const unsigned int wingVertexBytes =
        wingVertexCount * sizeof(azel::DebugColorVertex);
    const unsigned int wingIndexBytes =
        wingVertexCount * sizeof(std::uint16_t);

    g_basicWingVertices = static_cast<azel::DebugColorVertex*>(
        probeGpuAlloc(
            wingVertexBytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_basicWingVertexUid));
    g_basicWingIndices = static_cast<std::uint16_t*>(
        probeGpuAlloc(
            wingIndexBytes,
            SCE_GXM_MEMORY_ATTRIB_READ,
            &g_basicWingIndexUid));

    if (!g_basicWingVertices || !g_basicWingIndices) {
        failure("[FAIL] BASIC WING GPU MEMORY");
        return;
    }

    std::memcpy(
        g_basicWingVertices,
        g_basicWingCpuMesh.vertices.data(),
        wingVertexBytes);
    for (unsigned int i = 0; i < wingVertexCount; ++i)
        g_basicWingIndices[i] = static_cast<std::uint16_t>(i);

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

    static const float identityWvp[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    sceGxmSetUniformDataF(
        wingUniformBuffer, g_probeWvpParam, 0, 16, identityWvp);

    const int wingStreamResult =
        sceGxmSetVertexStream(
            g_probeContext, 0, g_basicWingVertices);
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
        g_basicWingIndices,
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
    status("[PASS] GXM BASIC WING DISPLAY", 0xFF80E0FFu);
    std::printf("[GXM] Basic Wing GXM color buffer queued for display\n");
}

bool debug_console_visible()
{
    return g_debugVisible;
}

bool load_basic_wing_viewer()
{
    g_basicWingCpuMesh = {};
    if (!azel::build_basic_wing_debug_mesh(g_basicWingCpuMesh) ||
        g_basicWingCpuMesh.vertices.empty() ||
        g_basicWingCpuMesh.vertices.size() > 65535) {
        g_basicWingCpuReady = false;
        return false;
    }

    // Fixed presentation view for the first real model draw. Keep all camera
    // work on the CPU so the proven identity-WVP GXM path remains unchanged.
    constexpr float yaw = 0.60f;
    constexpr float pitch = -0.30f;
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);

    for (auto& v : g_basicWingCpuMesh.vertices) {
        const float x0 = v.x;
        const float y0 = v.y;
        const float z0 = v.z;

        const float x1 = x0 * cy + z0 * sy;
        const float z1 = -x0 * sy + z0 * cy;
        const float y1 = y0 * cp - z1 * sp;
        const float z2 = y0 * sp + z1 * cp;

        v.x = x1;
        v.y = y1;
        v.z = z2;
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
    const float cy0 = (minY + maxY) * 0.5f;
    const float cz = (minZ + maxZ) * 0.5f;
    const float extentX = std::max(maxX - minX, 0.001f);
    const float extentY = std::max(maxY - minY, 0.001f);
    const float extentZ = std::max(maxZ - minZ, 0.001f);
    const float maxExtent = std::max(extentX, std::max(extentY, extentZ));
    const float scale = 1.45f / maxExtent;

    for (auto& v : g_basicWingCpuMesh.vertices) {
        v.x = (v.x - cx) * scale;
        v.y = (v.y - cy0) * scale;
        // Keep depth comfortably inside clip/depth range.
        v.z = (v.z - cz) * scale * 0.5f;
    }

    g_basicWingCpuReady = true;
    return true;
}

void begin_frame()
{
    fill(0xFF181818u);

    if (!g_debugVisible)
        return;

    drawText(32, 24, "LAGI - PDS VITA RUNTIME", 0xFFFFFFFFu, 2);
    drawText(32, 48, "BOOT / INTEGRATION STATUS", 0xFFB0B0B0u, 1);

    int y = 68;
    for (int i = 0; i < g_statusCount; ++i) {
        drawText(40, y, g_status[i].text, g_status[i].color, 1);
        y += 11;
    }

    if (g_azelAlive)
        drawText(40, y + 4, "TASK LOOP: ACTIVE", 0xFF30E030u, 1);
}

void end_frame()
{
    if (g_probeDisplayingGxm)
        return;

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = g_frameBuffer[g_drawBuffer];
    fb.pitch = kPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

    // Queue the fully rendered back buffer for the next scanout, then wait
    // for vblank before switching which buffer the CPU draws into.
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    g_drawBuffer ^= 1;
}

} // namespace lagi::platform::renderer
