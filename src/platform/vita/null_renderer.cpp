#include "lagi/platform.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace lagi::platform::renderer {

static constexpr int kWidth = 960;
static constexpr int kHeight = 544;
static constexpr int kPitch = 960;
static constexpr std::size_t kFrameBytes =
    static_cast<std::size_t>(kPitch) * kHeight * sizeof(std::uint32_t);
static constexpr int kMaxStatus = 20;

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

void toggle_debug_console()
{
    // Stage 3 native GXM probe: initialize, map the core ring buffers, then
    // create only the GXM context. Do not create render targets, surfaces,
    // shader patchers, shader programs, or submit a scene.
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
        std::printf("[GXM] sceGxmCreateContext failed: 0x%08X\n",
                    static_cast<unsigned int>(contextResult));
        return;
    }

    status("[PASS] GXM CONTEXT", 0xFF80E0FFu);
    std::printf("[GXM] context creation passed\n");
}

bool debug_console_visible()
{
    return g_debugVisible;
}

bool load_basic_wing_viewer()
{
    // Deliberately non-fatal isolation build: do not execute any GXM calls.
    return false;
}

void begin_frame()
{
    fill(0xFF181818u);

    if (!g_debugVisible)
        return;

    drawText(32, 24, "LAGI - PDS VITA RUNTIME", 0xFFFFFFFFu, 3);
    drawText(32, 58, "BOOT / INTEGRATION STATUS", 0xFFB0B0B0u, 2);

    int y = 92;
    for (int i = 0; i < g_statusCount; ++i) {
        drawText(40, y, g_status[i].text, g_status[i].color, 2);
        y += 22;
    }

    if (g_azelAlive)
        drawText(40, y + 8, "TASK LOOP: ACTIVE", 0xFF30E030u, 2);
}

void end_frame()
{
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
