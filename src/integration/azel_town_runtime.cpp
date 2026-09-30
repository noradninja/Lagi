#include "lagi/azel_town_runtime.h"

#include "lagi/azel_town_bootstrap.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

s8 readSaturnS8(sSaturnPtr ptr);
s32 readSaturnS32(sSaturnPtr ptr);
u32 readSaturnU32(sSaturnPtr ptr);
fixedPoint readSaturnFP(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);

namespace lagi::azel {

static TownRuntimeState g_runtime{};

const TownRuntimeState& town_runtime()
{
    return g_runtime;
}

const TownRuntimeCell* town_runtime_active_cell()
{
    if (!g_runtime.initialized ||
        g_runtime.activeCellIndex < 0 ||
        g_runtime.activeCellIndex >=
            static_cast<int>(g_runtime.cells.size()))
        return nullptr;

    const TownRuntimeCell& cell =
        g_runtime.cells[static_cast<std::size_t>(g_runtime.activeCellIndex)];
    return cell.valid ? &cell : nullptr;
}

const std::vector<std::uint8_t>* town_runtime_resource(const char* name)
{
    if (!name)
        return nullptr;

    for (const auto& resource : g_runtime.resources) {
        if (resource.name == name)
            return &resource.bytes;
    }
    return nullptr;
}

static bool load_runtime_resource(const char* name)
{
    TownRuntimeResource resource{};
    resource.name = name;
    if (!lagi::disc::read_file(name, resource.bytes) ||
        resource.bytes.empty()) {
        lagi::platform::logging::writef(
            "[TownRuntime] failed resource load: %s\n", name);
        return false;
    }

    lagi::platform::logging::writef(
        "[TownRuntime] resource %-12s %u bytes\n",
        name,
        static_cast<unsigned>(resource.bytes.size()));
    g_runtime.resources.push_back(std::move(resource));
    return true;
}

void update_town_runtime_active_cell(float worldX, float worldZ)
{
    if (!g_runtime.initialized || g_runtime.cells.empty()) {
        g_runtime.activeCellIndex = -1;
        return;
    }

    // The first Ruins scene is a 1x1 grid. For future multi-cell towns this
    // already centralizes selection in the scene runtime. Until the exact
    // Azel ring-buffer grid task is ported, choose the closest valid cell
    // origin in X/Z rather than letting rendering code own cell selection.
    int best = -1;
    float bestDist2 = 0.0f;
    for (std::size_t i = 0; i < g_runtime.cells.size(); ++i) {
        const auto& cell = g_runtime.cells[i];
        if (!cell.valid)
            continue;

        const float dx = worldX - cell.origin[0];
        const float dz = worldZ - cell.origin[2];
        const float d2 = dx*dx + dz*dz;
        if (best < 0 || d2 < bestDist2) {
            best = static_cast<int>(i);
            bestDist2 = d2;
        }
    }
    g_runtime.activeCellIndex = best;
}

bool init_town_runtime()
{
    g_runtime = {};

    const TownBootstrapInfo& boot = town_bootstrap_info();
    sSaturnMemoryFile* overlay = town_overlay_file();
    if (!boot.valid || !overlay || !overlay->m_data) {
        lagi::platform::logging::writef(
            "[TownRuntime] bootstrap/overlay unavailable\n");
        return false;
    }

    g_runtime.overlayFile = boot.overlayFile;
    g_runtime.setupNpcFileIndex = boot.setupNpcFileIndex;
    g_runtime.gridWidth = boot.gridWidth;
    g_runtime.gridHeight = boot.gridHeight;
    g_runtime.gridCellSize =
        static_cast<float>(boot.gridCellSize) / 65536.0f;
    g_runtime.initialScriptEA = 0x06054398u;
    g_runtime.scriptTableEA = boot.scriptTableEA;
    g_runtime.edgeEA = boot.edgeEA;
    g_runtime.envLcsTargetCount = boot.envLcsTargetCount;
    g_runtime.envLcsTargetEA = boot.envLcsTargetEA;

    // readTownSetup(..., 12) in TWN_RUIN_data.
    constexpr unsigned kRuinScriptCount = 12;
    const sSaturnPtr scriptTable =
        overlay->getSaturnPtr(boot.scriptTableEA);
    g_runtime.townScripts.reserve(kRuinScriptCount);
    for (unsigned i = 0; i < kRuinScriptCount; ++i) {
        const sSaturnPtr script =
            readSaturnEA(scriptTable + i * 4u);
        g_runtime.townScripts.push_back(
            static_cast<std::uint32_t>(script.m_offset));
    }

    const unsigned cellCount =
        static_cast<unsigned>(boot.gridWidth) *
        static_cast<unsigned>(boot.gridHeight);
    g_runtime.cells.resize(cellCount);

    const sSaturnPtr grid =
        overlay->getSaturnPtr(boot.gridEA);
    unsigned validCells = 0;
    for (unsigned i = 0; i < cellCount; ++i) {
        const sSaturnPtr cell =
            readSaturnEA(grid + i * 4u);

        TownRuntimeCell& dst = g_runtime.cells[i];
        if (cell.isNull())
            continue;

        dst.ea = static_cast<std::uint32_t>(cell.m_offset);
        dst.origin[0] =
            static_cast<float>(readSaturnS32(cell + 0x00)) / 65536.0f;
        dst.origin[1] =
            static_cast<float>(readSaturnS32(cell + 0x04)) / 65536.0f;
        dst.origin[2] =
            static_cast<float>(readSaturnS32(cell + 0x08)) / 65536.0f;
        dst.staticObjectListEA =
            static_cast<std::uint32_t>(
                readSaturnEA(cell + 0x0C).m_offset);
        dst.collisionListEA =
            static_cast<std::uint32_t>(
                readSaturnEA(cell + 0x14).m_offset);
        dst.valid = true;
        ++validCells;
    }

    // Match overlayStart_TWN_RUIN's scene-owned file set. Keeping the bytes
    // here is the first step away from each debug adapter re-reading the disc.
    static constexpr std::array<const char*, 7> kResources = {{
        "COMMON3.MCB",
        "COMMON3.CGB",
        "RUINMP.MCB",
        "RUINMP.CGB",
        "RUINSCR.SCB",
        "RUINSCR.PNB",
        "EVTRUIN.FNT",
    }};

    g_runtime.resources.reserve(kResources.size());
    for (const char* name : kResources) {
        if (!load_runtime_resource(name))
            return false;
    }

    g_runtime.initialized = validCells != 0;
    if (!g_runtime.initialized)
        return false;

    update_town_runtime_active_cell(
        g_runtime.cells[0].origin[0],
        g_runtime.cells[0].origin[2]);

    lagi::platform::logging::writef(
        "[TownRuntime] %s scene owner ready: grid=%dx%d cells=%u/%u scripts=%u resources=%u active=%d\n",
        g_runtime.overlayFile.c_str(),
        static_cast<int>(g_runtime.gridWidth),
        static_cast<int>(g_runtime.gridHeight),
        validCells,
        cellCount,
        static_cast<unsigned>(g_runtime.townScripts.size()),
        static_cast<unsigned>(g_runtime.resources.size()),
        g_runtime.activeCellIndex);

    lagi::platform::renderer::status(
        "[PASS] TWN_RUIN NATIVE SCENE OWNER",
        0xFF70E0A0u);
    return true;
}

} // namespace lagi::azel
