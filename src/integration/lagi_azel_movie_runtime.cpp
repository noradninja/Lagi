#include "lagi/azel_movie_runtime_hooks.h"

#include "lagi/azel_movie_bridge.h"
#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"

#include <psp2/kernel/processmgr.h>

#include <cstdint>

namespace {

std::uint64_t g_lastMovieUpdateUs = 0;
std::uint64_t g_lastMoviePowerTickUs = 0;

void clear_backend_marker()
{
    fileInfoStruct.mC_gfsHandle = nullptr;
}

} // namespace

void lagiAzelMovieStreamOpen(const char* cpkFileName)
{
    clear_backend_marker();
    g_lastMovieUpdateUs = 0;
    g_lastMoviePowerTickUs = 0;

    lagi::platform::logging::writef(
        "[AzelMovie] open selected CPK=%s\n",
        cpkFileName ? cpkFileName : "<null>");

    if (!cpkFileName || !lagi::azel::movie_backend_open(cpkFileName)) {
        lagi::platform::logging::writef(
            "[AzelMovie] FAIL open CPK=%s error=%s\n",
            cpkFileName ? cpkFileName : "<null>",
            lagi::azel::movie_backend_error());
        lagi::platform::renderer::failure(
            "[FAIL] AZEL MOVIE BACKEND - SEE LOG");
        return;
    }

    fileInfoStruct.m20_videoWidth =
        lagi::azel::movie_backend_video_width();
    fileInfoStruct.m24_videoHeight =
        lagi::azel::movie_backend_video_height();

    // Preserve Azel's own movie-visible state. Neptune does not consume the
    // Saturn VRAM destination, but subtitle/window logic still reads these
    // dimensions and registers from the original movie task.
    const s32 width = static_cast<s32>(fileInfoStruct.m20_videoWidth);
    const s32 height = static_cast<s32>(fileInfoStruct.m24_videoHeight);
    const u32 vramDest = static_cast<u32>(
        (((height * -0x200 + 0x20160) - width) * 4) >> 1);
    fileInfoStruct.m1C_vramDest = vramDest + 0x25E00000u;

    s_VDP2Regs* const regs = vdp2Controls.m4_pendingVdp2Regs;
    if (regs) {
        regs->mC0_WPSX0 = static_cast<s16>(((0x160 - width) >> 1) << 1);
        regs->mC2_WPSY0 = static_cast<s16>((0xE0 - height) >> 1);
        regs->mC4_WPEX0 = static_cast<s16>(((width + 0x15F) >> 1) << 1);
        regs->mC6_WPEY0 = static_cast<s16>((height + 0xDF) >> 1);
    }

    fileInfoStruct.m18_decodeBuffer = nullptr;
    fileInfoStruct.m14_frameCount = 0;
    fileInfoStruct.m28_countdown = 0x3C;

    if (VDP2Regs_.m4_TVSTAT & 1)
        vblankData.m14_numVsyncPerFrame = 1;

    // Upstream movie.cpp treats this field as its stream-active predicate.
    // The Vita hook never dereferences it; actual ownership is in MoviePlayer.
    fileInfoStruct.mC_gfsHandle =
        reinterpret_cast<GfsHn*>(static_cast<std::uintptr_t>(1));

    g_lastMovieUpdateUs = sceKernelGetProcessTimeWide();

    lagi::platform::logging::writef(
        "[AzelMovie] backend active CPK=%s video=%ux%u\n",
        cpkFileName,
        static_cast<unsigned int>(fileInfoStruct.m20_videoWidth),
        static_cast<unsigned int>(fileInfoStruct.m24_videoHeight));
}

void lagiAzelMovieStreamClose()
{
    if (lagi::azel::movie_backend_active() ||
        lagi::azel::movie_backend_finished()) {
        lagi::platform::logging::writef(
            "[AzelMovie] close pts=%llu\n",
            static_cast<unsigned long long>(
                lagi::azel::movie_backend_pts()));
    }

    lagi::azel::movie_backend_close();
    clear_backend_marker();
    g_lastMovieUpdateUs = 0;
    g_lastMoviePowerTickUs = 0;

    // A Saturn Start edge used to skip a movie must be consumed by the movie
    // task. runTasks() can continue into newly-created gameplay/menu tasks in
    // the same host frame; leaving the edge live makes the first town frame
    // interpret the same press as "open pause menu".
    auto& input =
        graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
    const bool consumedStart =
        (input.m8_newButtonDown & 0x0008u) != 0u ||
        (input.mC_newButtonDown2 & 0x0008u) != 0u;
    input.m8_newButtonDown &= static_cast<u16>(~0x0008u);
    input.mC_newButtonDown2 &= static_cast<u16>(~0x0008u);
    if (consumedStart) {
        lagi::platform::logging::writef(
            "[AzelMovie] consumed Start skip edge at movie handoff\n");
    }

    if (VDP2Regs_.m4_TVSTAT & 1)
        vblankData.m14_numVsyncPerFrame = 2;
}

std::uint32_t lagiAzelMovieLastUpdate()
{
    if (!fileInfoStruct.mC_gfsHandle)
        return 0;

    const std::uint64_t now = sceKernelGetProcessTimeWide();
    const std::uint64_t elapsed =
        g_lastMovieUpdateUs ? now - g_lastMovieUpdateUs : 0;
    g_lastMovieUpdateUs = now;

    lagi::azel::movie_backend_update(elapsed);
    fileInfoStruct.m14_frameCount =
        static_cast<u32>(lagi::azel::movie_backend_pts());

    if (lagi::azel::movie_backend_active()) {
        // Playback has no pad activity. Refresh idle timers once a second,
        // scoped to the active backend; no persistent power lock to release.
        constexpr std::uint64_t kPowerTickIntervalUs = 1000000u;
        if (!g_lastMoviePowerTickUs ||
            now - g_lastMoviePowerTickUs >= kPowerTickIntervalUs) {
            const int tickResult = sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
            if (!g_lastMoviePowerTickUs) {
                lagi::platform::logging::writef(
                    "[AzelMovie] keepAwake=power-tick intervalUs=1000000 result=0x%08X\n",
                    static_cast<unsigned int>(tickResult));
            }
            g_lastMoviePowerTickUs = now;
        }
        return 0;
    }

    if (lagi::azel::movie_backend_finished()) {
        lagi::platform::logging::writef(
            "[AzelMovie] backend finished pts=%llu\n",
            static_cast<unsigned long long>(
                lagi::azel::movie_backend_pts()));
    } else {
        lagi::platform::logging::writef(
            "[AzelMovie] FAIL decode error=%s\n",
            lagi::azel::movie_backend_error());
        lagi::platform::renderer::failure(
            "[FAIL] AZEL MOVIE DECODE - SEE LOG");
    }

    // Match the original lastUpdateFunction contract: once the decoder ends,
    // closeMovieStream makes mC_gfsHandle null and s_movieMainWorkArea::Draw
    // advances its own authentic state machine on the next task pass.
    lagiAzelMovieStreamClose();
    return 0;
}
