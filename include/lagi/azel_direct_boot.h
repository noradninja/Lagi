#pragma once

#include <cstdint>
#include <string>

namespace lagi::azel {

struct DirectBootTarget {
    std::uint8_t gameStatus = 0;
    std::uint8_t gameMode = 0;
    std::uint8_t gameModeEntry = 0;

    std::int8_t townOverlayIndex = -1;
    std::int8_t townEntryIndex = -1;
    std::string overlayFile;

    bool resolved = false;
    bool overlayPresent = false;
};

// Resolve the first post-name-entry/movie 3D scene using the same COMMON.DAT
// dispatch data consumed by Azel's module manager/town loader.
bool init_direct_boot_target();

const DirectBootTarget& direct_boot_target();

} // namespace lagi::azel
