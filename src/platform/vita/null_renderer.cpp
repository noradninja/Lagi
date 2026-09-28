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

void toggle_debug_console()
{
    // Stage 1 native GXM probe: initialize only. Do not create a context,
    // render target, shader patcher, or submit a scene yet.
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

    const int result = sceGxmInitialize(&params);
    if (result < 0) {
        char line[78];
        std::snprintf(line, sizeof(line), "[FAIL] GXM INIT 0X%08X",
                      static_cast<unsigned int>(result));
        failure(line);
        std::printf("[GXM] sceGxmInitialize failed: 0x%08X\n",
                    static_cast<unsigned int>(result));
        return;
    }

    g_gxmInitialized = true;
    status("[PASS] GXM INITIALIZE", 0xFF80E0FFu);
    std::printf("[GXM] sceGxmInitialize passed\n");
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
