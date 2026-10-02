#pragma once

#include "lagi/azel_compat.h"

#include <cstdint>
#include <string>
#include <vector>

namespace lagi::azel {

struct TownBootstrapInfo {
    std::string overlayFile;
    std::uint32_t overlayBytes = 0;

    std::int8_t setupNpcFileIndex = -1;
    std::int8_t gridWidth = 0;
    std::int8_t gridHeight = 0;
    std::int32_t gridCellSize = 0;
    std::uint32_t gridEA = 0;
    std::int32_t envLcsTargetCount = 0;
    std::uint32_t envLcsTargetEA = 0;
    std::uint32_t scriptTableEA = 0;
    std::uint32_t edgeEA = 0;

    bool requiredAssetsPresent = false;
    bool valid = false;
};

bool init_town_bootstrap();
const TownBootstrapInfo& town_bootstrap_info();
sSaturnMemoryFile* town_overlay_file();

} // namespace lagi::azel
