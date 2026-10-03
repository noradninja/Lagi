#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct sProcessed3dModel;

namespace lagi::azel {

struct TownRuntimeCollisionVertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct TownRuntimeCollisionQuad {
    std::uint16_t indices[4]{};
    float normal[3]{};
    std::uint16_t cmdSrca = 0;
    std::uint16_t onCollisionScriptIndex = 0;
};

struct TownRuntimeCollisionModel {
    float radius = 0.0f;
    std::vector<TownRuntimeCollisionVertex> vertices;
    std::vector<TownRuntimeCollisionQuad> quads;
};

struct TownRuntimeCollisionInstance {
    std::uint32_t modelTableOffset = 0;
    float position[3]{};
    TownRuntimeCollisionModel model;
};

struct TownRuntimeCell {
    std::uint32_t ea = 0;
    float origin[3]{};
    std::uint32_t staticObjectListEA = 0;
    std::uint32_t billboardListEA = 0;
    std::uint32_t collisionListEA = 0;
    std::vector<TownRuntimeCollisionInstance> collisionInstances;
    bool valid = false;
};

struct TownRuntimeResource {
    std::string name;
    std::vector<std::uint8_t> bytes;
};

struct TownRuntimeBundle {
    struct ModelCache;
    std::int8_t fileIndex = -1;
    const TownRuntimeResource* model = nullptr;
    const TownRuntimeResource* graphics = nullptr;
    std::uint16_t vdp1Base = 0;
    std::uint16_t vdp1SizeUnits = 0;
    unsigned refCount = 0;
    std::shared_ptr<ModelCache> modelCache;
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
    std::vector<TownRuntimeBundle> bundles;
};

bool init_town_runtime();
const TownRuntimeState& town_runtime();
const TownRuntimeCell* town_runtime_active_cell();
const std::vector<std::uint8_t>* town_runtime_resource(const char* name);

// Transitional world-grid update. TWN_RUIN is 1x1 today, but this keeps the
// scene owner responsible for selecting the active cell rather than the
// renderer/debug reconstruction doing so directly.
void update_town_runtime_active_cell(float worldX, float worldZ);
bool acquire_town_runtime_bundle(std::int8_t fileIndex);
void release_town_runtime_bundle(std::int8_t fileIndex);
const TownRuntimeBundle* town_runtime_bundle(std::int8_t fileIndex);
sProcessed3dModel* town_runtime_model(
    std::int8_t fileIndex,
    std::uint32_t tableOffset);

} // namespace lagi::azel
