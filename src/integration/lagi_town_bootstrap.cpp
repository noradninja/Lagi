#include "lagi/lagi_town_bootstrap.h"

#include "lagi/lagi_direct_boot.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

#include <array>
#include <cstdio>
#include <vector>

// Implemented by lagi_saturn_memory_vita.cpp.
s8 readSaturnS8(sSaturnPtr ptr);
s32 readSaturnS32(sSaturnPtr ptr);
u32 readSaturnU32(sSaturnPtr ptr);
fixedPoint readSaturnFP(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);

namespace lagi::azel {

static std::vector<u8> g_overlayStorage;
static sSaturnMemoryFile g_overlayFile;
static TownBootstrapInfo g_info{};

const TownBootstrapInfo& town_bootstrap_info()
{
    return g_info;
}

sSaturnMemoryFile* town_overlay_file()
{
    return g_info.valid ? &g_overlayFile : nullptr;
}

static bool ptr_in_overlay(u32 ea, u32 bytes = 1)
{
    if (ea < g_overlayFile.m_base)
        return false;

    const u32 offset = ea - g_overlayFile.m_base;
    return offset <= g_overlayFile.m_dataSize &&
           bytes <= g_overlayFile.m_dataSize - offset;
}

bool init_town_bootstrap()
{
    g_info = {};
    g_overlayStorage.clear();
    g_overlayFile = {};

    const DirectBootTarget& target = direct_boot_target();
    if (!target.resolved || !target.overlayPresent) {
        lagi::platform::logging::writef(
            "[TownBoot] direct-boot target is not ready\n");
        return false;
    }

    g_info.overlayFile = target.overlayFile;

    if (!lagi::disc::read_file(
            g_info.overlayFile.c_str(), g_overlayStorage) ||
        g_overlayStorage.empty()) {
        lagi::platform::logging::writef(
            "[TownBoot] failed to load %s\n",
            g_info.overlayFile.c_str());
        return false;
    }

    g_overlayFile.m_name = g_info.overlayFile;
    g_overlayFile.m_data = g_overlayStorage.data();
    g_overlayFile.m_dataSize =
        static_cast<u32>(g_overlayStorage.size());
    g_overlayFile.m_base = 0x06054000u;
    g_info.overlayBytes = g_overlayFile.m_dataSize;

    // TWN_RUIN_data constructor:
    //   mTownSetups.push_back(readTownSetup(getSaturnPtr(0x605E984), 12));
    constexpr u32 kTownSetupEA = 0x0605E984u;
    constexpr u32 kInitialScriptEA = 0x06054398u;
    constexpr u32 kEdgeEA = 0x0605E990u;

    if (!ptr_in_overlay(kTownSetupEA, 12) ||
        !ptr_in_overlay(kInitialScriptEA, 4) ||
        !ptr_in_overlay(kEdgeEA, 4)) {
        lagi::platform::logging::writef(
            "[TownBoot] known TWN_RUIN addresses fall outside overlay (%u bytes)\n",
            g_overlayFile.m_dataSize);
        return false;
    }

    const sSaturnPtr townSetup =
        g_overlayFile.getSaturnPtr(kTownSetupEA);

    g_info.setupNpcFileIndex = readSaturnS8(townSetup + 0);
    const sSaturnPtr gridSetup = readSaturnEA(townSetup + 4);
    g_info.scriptTableEA =
        static_cast<u32>(readSaturnEA(townSetup + 8).m_offset);
    g_info.edgeEA = kEdgeEA;

    if (!ptr_in_overlay(static_cast<u32>(gridSetup.m_offset), 0x14) ||
        !ptr_in_overlay(g_info.scriptTableEA, 12u * 4u)) {
        lagi::platform::logging::writef(
            "[TownBoot] town setup points outside TWN_RUIN.PRG: grid=%08X scripts=%08X\n",
            static_cast<unsigned>(gridSetup.m_offset),
            static_cast<unsigned>(g_info.scriptTableEA));
        return false;
    }

    g_info.gridWidth = readSaturnS8(gridSetup + 0);
    g_info.gridHeight = readSaturnS8(gridSetup + 1);
    g_info.gridCellSize = readSaturnFP(gridSetup + 4).asS32();
    g_info.gridEA =
        static_cast<u32>(readSaturnEA(gridSetup + 8).m_offset);
    g_info.envLcsTargetCount = readSaturnS32(gridSetup + 0x0C);
    g_info.envLcsTargetEA =
        static_cast<u32>(readSaturnEA(gridSetup + 0x10).m_offset);

    if (g_info.gridWidth <= 0 ||
        g_info.gridHeight <= 0 ||
        g_info.gridCellSize <= 0 ||
        !ptr_in_overlay(g_info.gridEA, 4)) {
        lagi::platform::logging::writef(
            "[TownBoot] invalid town grid: %dx%d cell=%08X gridEA=%08X\n",
            static_cast<int>(g_info.gridWidth),
            static_cast<int>(g_info.gridHeight),
            static_cast<unsigned>(g_info.gridCellSize),
            static_cast<unsigned>(g_info.gridEA));
        return false;
    }

    // Exact asset list used by overlayStart_TWN_RUIN(), plus its immediate
    // background/font resources. We only preflight presence here; loading
    // into VDP1/VDP2 memory comes in the execution stage.
    static constexpr std::array<const char*, 7> requiredAssets = {{
        "COMMON3.MCB",
        "COMMON3.CGB",
        "RUINMP.MCB",
        "RUINMP.CGB",
        "RUINSCR.SCB",
        "RUINSCR.PNB",
        "EVTRUIN.FNT",
    }};

    g_info.requiredAssetsPresent = true;
    for (const char* asset : requiredAssets) {
        if (!lagi::disc::has_file(asset)) {
            g_info.requiredAssetsPresent = false;
            lagi::platform::logging::writef(
                "[TownBoot] missing required asset: %s\n", asset);
        }
    }

    g_info.valid = g_info.requiredAssetsPresent;

    lagi::platform::logging::writef(
        "[TownBoot] %s base=06054000 size=%u setupNPC=%d grid=%dx%d cell=%08X gridEA=%08X scripts=%08X edge=%08X LCS=%d@%08X assets=%s\n",
        g_info.overlayFile.c_str(),
        static_cast<unsigned>(g_info.overlayBytes),
        static_cast<int>(g_info.setupNpcFileIndex),
        static_cast<int>(g_info.gridWidth),
        static_cast<int>(g_info.gridHeight),
        static_cast<unsigned>(g_info.gridCellSize),
        static_cast<unsigned>(g_info.gridEA),
        static_cast<unsigned>(g_info.scriptTableEA),
        static_cast<unsigned>(g_info.edgeEA),
        static_cast<int>(g_info.envLcsTargetCount),
        static_cast<unsigned>(g_info.envLcsTargetEA),
        g_info.requiredAssetsPresent ? "OK" : "MISSING");

    if (!g_info.valid) {
        lagi::platform::renderer::failure(
            "[FAIL] TWN_RUIN OVERLAY PREFLIGHT");
        return false;
    }

    char status[78];
    std::snprintf(
        status, sizeof(status),
        "[PASS] TOWN OVERLAY %.24s %dx%d GRID",
        g_info.overlayFile.c_str(),
        static_cast<int>(g_info.gridWidth),
        static_cast<int>(g_info.gridHeight));
    lagi::platform::renderer::status(status, 0xFF70E0A0u);

    return true;
}

} // namespace lagi::azel
