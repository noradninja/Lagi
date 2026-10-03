#include "lagi/lagi_movie_sequence.h"

#include "lagi/azel_movie_bridge.h"
#include "lagi/disc_image.h"
#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_direct_boot.h"
#include "lagi/platform.h"

#include "kernel/moduleManager.h"

#include <psp2/kernel/processmgr.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>

// Reconstructed in Azel's moduleManager.cpp. Calling the original routine
// preserves the status flags and post-movie transition instead of duplicating
// that game logic in the Vita adapter.
s32 exitMenuTaskSub1TaskDrawSub1(p_workArea pWorkArea, s32 index);

namespace lagi::azel {
namespace {

enum class SequenceState {
    Town,
    Playing,
    Complete,
    Failed,
};

// Azel game-status table:
//   status 0x05 -> movie timing index 0x02
// Azel movie timing table:
//   index 0x02 -> CPK 0x06, count 2
// Azel CPK table:
//   0x06 EVT004_1.CPK, 0x07 EVT004_2.CPK
//
// This belongs to the direct-boot sequencing adapter, not MoviePlayer: the
// generic Lagi backend continues to open exactly the filename Azel selects.
constexpr std::uint16_t kElevatorMovieGameStatus = 0x05;
constexpr s32 kElevatorMovieTimingIndex = 0x02;
constexpr const char* kElevatorMovieFiles[] = {
    "EVT004_1.CPK",
    "EVT004_2.CPK",
};

SequenceState g_state = SequenceState::Town;
std::size_t g_fileIndex = 0;
std::uint64_t g_lastUpdateUs = 0;

void report_failure(const char* context)
{
    const char* error = movie_backend_error();
    platform::logging::writef(
        "[MovieSequence] FAIL %s file=%s error=%s\n",
        context ? context : "playback",
        g_fileIndex < (sizeof(kElevatorMovieFiles) /
                       sizeof(kElevatorMovieFiles[0]))
            ? kElevatorMovieFiles[g_fileIndex]
            : "<none>",
        error && error[0] ? error : "unknown movie backend error");
    platform::renderer::failure("[FAIL] ELEVATOR MOVIE - SEE LOG");
    movie_backend_close();
    g_state = SequenceState::Failed;
}

bool open_current_file()
{
    const char* file = kElevatorMovieFiles[g_fileIndex];
    platform::logging::writef(
        "[MovieSequence] open Azel movie=%d part=%u/%u file=%s\n",
        static_cast<int>(kElevatorMovieTimingIndex),
        static_cast<unsigned int>(g_fileIndex + 1u),
        static_cast<unsigned int>(
            sizeof(kElevatorMovieFiles) / sizeof(kElevatorMovieFiles[0])),
        file);

    if (!movie_backend_open(file)) {
        report_failure("open");
        return false;
    }

    char status[78]{};
    std::snprintf(
        status,
        sizeof(status),
        "[PASS] MOVIE %.40s OPEN",
        file);
    platform::renderer::status(status, 0xFF70E0A0u);
    g_lastUpdateUs = sceKernelGetProcessTimeWide();
    return true;
}

bool begin_elevator_movie()
{
    for (const char* file : kElevatorMovieFiles) {
        if (!disc::has_file(file)) {
            platform::logging::writef(
                "[MovieSequence] required CPK missing from mounted disc: %s\n",
                file);
            platform::renderer::failure(
                "[FAIL] ELEVATOR CPK MISSING - SEE LOG");
            g_state = SequenceState::Failed;
            return false;
        }
    }

    // Mirror moduleManager_Draw's handoff before dispatching movie index 2.
    gGameStatus.m6_previousGameStatus = gGameStatus.m4_gameStatus;
    gGameStatus.m4_gameStatus = kElevatorMovieGameStatus;
    gGameStatus.m8_nextGameStatus = 0;
    g_fileIndex = 0;
    g_state = SequenceState::Playing;

    platform::logging::writef(
        "[MovieSequence] Azel status 0x%02X -> movie index %d\n",
        static_cast<unsigned int>(kElevatorMovieGameStatus),
        static_cast<int>(kElevatorMovieTimingIndex));
    return open_current_file();
}

void finish_elevator_movie()
{
    movie_backend_close();
    g_state = SequenceState::Complete;

    // This is Azel's original completion path for game status 5. It sets the
    // relevant story bit and requests status 0x50 (above Excavation).
    exitMenuTaskSub1TaskDrawSub1(nullptr, kElevatorMovieGameStatus);
    platform::logging::writef(
        "[MovieSequence] complete; Azel requested next status 0x%02X\n",
        static_cast<unsigned int>(gGameStatus.m8_nextGameStatus));
    platform::renderer::status(
        "[PASS] ELEVATOR MOVIE SEQUENCE COMPLETE",
        0xFF70E0A0u);
}

} // namespace

bool init_movie_sequence_adapter()
{
    movie_backend_close();
    g_state = SequenceState::Town;
    g_fileIndex = 0;
    g_lastUpdateUs = 0;

    const DirectBootTarget& target = direct_boot_target();
    if (!target.resolved)
        return false;

    // Direct boot enters the same game status that the real module manager
    // would have published before loading TWN_RUIN.
    gGameStatus.m4_gameStatus = target.gameStatus;
    gGameStatus.m6_previousGameStatus = 0;
    gGameStatus.m8_nextGameStatus = 0;
    platform::logging::writef(
        "[MovieSequence] direct-boot Azel status initialized to 0x%02X\n",
        static_cast<unsigned int>(gGameStatus.m4_gameStatus));
    return true;
}

bool service_movie_sequence_adapter()
{
    if (g_state == SequenceState::Town) {
        if (gGameStatus.m8_nextGameStatus != kElevatorMovieGameStatus)
            return false;
        begin_elevator_movie();
    }

    if (g_state == SequenceState::Failed ||
        g_state == SequenceState::Complete)
        return true;

    const std::uint64_t now = sceKernelGetProcessTimeWide();
    const std::uint64_t elapsed = g_lastUpdateUs ? now - g_lastUpdateUs : 0;
    g_lastUpdateUs = now;
    movie_backend_update(elapsed);

    if (movie_backend_active())
        return true;

    if (!movie_backend_finished()) {
        report_failure("decode");
        return true;
    }

    platform::logging::writef(
        "[MovieSequence] finished file=%s pts=%llu\n",
        kElevatorMovieFiles[g_fileIndex],
        static_cast<unsigned long long>(movie_backend_pts()));
    movie_backend_close();

    ++g_fileIndex;
    if (g_fileIndex <
        sizeof(kElevatorMovieFiles) / sizeof(kElevatorMovieFiles[0])) {
        open_current_file();
        return true;
    }

    finish_elevator_movie();
    return true;
}

} // namespace lagi::azel
