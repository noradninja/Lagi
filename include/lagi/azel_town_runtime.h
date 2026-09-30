#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lagi::azel {

struct TownRuntimeCell {
    std::uint32_t ea = 0;
    float origin[3]{};
    std::uint32_t staticObjectListEA = 0;
    std::uint32_t collisionListEA = 0;
    bool valid = false;
};

struct TownRuntimeResource {
    std::string name;
    std::vector<std::uint8_t> bytes;
};

struct TownRuntimeState {
    bool initialized = false;
    std::string overlayFile;

    std::int8_t setupNpcFileIndex = -1;
    std::int8_t gridWidth = 0;
    std::int8_t gridHeight = 0;
    float gridCellSize = 0.0f;

    std::uint32_t initialScriptEA = 0;
    std::uint32_t scriptTableEA = 0;
    std::vector<std::uint32_t> townScripts;
    std::uint32_t edgeEA = 0;

    std::int32_t envLcsTargetCount = 0;
    std::uint32_t envLcsTargetEA = 0;

    std::vector<TownRuntimeCell> cells;
    int activeCellIndex = -1;

    std::vector<TownRuntimeResource> resources;
};

bool init_town_runtime();
const TownRuntimeState& town_runtime();
const TownRuntimeCell* town_runtime_active_cell();
const std::vector<std::uint8_t>* town_runtime_resource(const char* name);

// Transitional world-grid update. TWN_RUIN is 1x1 today, but this keeps the
// scene owner responsible for selecting the active cell rather than the
// renderer/debug reconstruction doing so directly.
void update_town_runtime_active_cell(float worldX, float worldZ);

} // namespace lagi::azel
