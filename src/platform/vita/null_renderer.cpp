#include "lagi/platform.h"

#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <cstdint>
#include <cstring>

namespace lagi::platform::renderer {

static constexpr int kWidth = 960;
static constexpr int kHeight = 544;
static constexpr int kPitch = 960;
static constexpr std::size_t kFrameBytes =
    static_cast<std::size_t>(kPitch) * kHeight * sizeof(std::uint32_t);

static SceUID g_frameMem = -1;
static std::uint32_t* g_frameBuffer = nullptr;
static bool g_azelAlive = false;
static bool g_discAlive = false;

static void fill(std::uint32_t color)
{
    if (!g_frameBuffer)
        return;

    for (int y = 0; y < kHeight; ++y) {
        std::uint32_t* row = g_frameBuffer + y * kPitch;
        for (int x = 0; x < kWidth; ++x)
            row[x] = color;
    }
}

bool init()
{
    const std::size_t allocSize = (kFrameBytes + 0x3FFFFu) & ~0x3FFFFu;

    g_frameMem = sceKernelAllocMemBlock(
        "LagiFramebuffer",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
        allocSize,
        nullptr);

    if (g_frameMem < 0)
        return false;

    void* base = nullptr;
    if (sceKernelGetMemBlockBase(g_frameMem, &base) < 0 || !base) {
        sceKernelFreeMemBlock(g_frameMem);
        g_frameMem = -1;
        return false;
    }

    g_frameBuffer = static_cast<std::uint32_t*>(base);
    fill(0xFF400000u); // dark blue in A8B8G8R8

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = g_frameBuffer;
    fb.pitch = kPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

    return sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME) >= 0;
}

void shutdown()
{
    if (g_frameMem >= 0) {
        sceKernelFreeMemBlock(g_frameMem);
        g_frameMem = -1;
    }
    g_frameBuffer = nullptr;
}

void set_azel_alive(bool alive)
{
    g_azelAlive = alive;
}

void set_disc_alive(bool alive)
{
    g_discAlive = alive;
}

void begin_frame()
{
    // Blue means the platform/framebuffer path is alive.
    // Green means Azel's task scheduler has completed Update + Draw.
    // Cyan means the real PDS disc image mounted and COMMON.DAT was decoded.
    if (g_discAlive)
        fill(0xFFA08000u);
    else
        fill(g_azelAlive ? 0xFF00A000u : 0xFF400000u);
}

void end_frame()
{
    sceDisplayWaitVblankStart();
}

} // namespace lagi::platform::renderer
