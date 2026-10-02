#include "lagi/lagi_direct_boot.h"

#include "lagi/lagi_compat.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include "commonOverlay.h"

#include <cstdio>

// Implemented by lagi_saturn_memory_vita.cpp. Keep these narrow declarations
// here instead of including Azel common.h, which drags in unrelated task/VDP2
// definitions that the direct-boot resolver does not need.
s8 readSaturnS8(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);
std::string readSaturnString(sSaturnPtr ptr);

namespace lagi::azel {

static DirectBootTarget g_target{};

const DirectBootTarget& direct_boot_target()
{
    return g_target;
}

bool init_direct_boot_target()
{
    g_target = {};

    if (!gCommonFile || !gCommonFile->m_data || gCommonFile->m_dataSize == 0) {
        lagi::platform::logging::writef(
            "[DirectBoot] COMMON.DAT is not initialized\n");
        return false;
    }

    // Pinned Azel moduleManager.cpp:
    //   status 0x01 -> movie 0
    //   status 0x02 -> FLD_D5 name entry
    //   status 0x03 -> movie 1
    //   status 0x04 -> game mode 1, town entry 0x10
    //
    // This is the first 3D scene after the startup/name/movie chain.
    g_target.gameStatus = 0x04;
    g_target.gameMode = 1;
    g_target.gameModeEntry = 0x10;

    // Pinned Azel town.cpp loadTownSub():
    //   pair = COMMON[0x2166E4 + townEntry * 2]
    //   loadTownPrg(pair[0], pair[1])
    //
    // loadTownPrg() resolves the overlay filename through:
    //   readEA(COMMON[0x2165D8 + overlayIndex * 16])
    const sSaturnPtr townDispatch =
        gCommonFile->getSaturnPtr(
            0x002166E4u +
            static_cast<u32>(g_target.gameModeEntry) * 2u);

    g_target.townOverlayIndex = readSaturnS8(townDispatch + 0);
    g_target.townEntryIndex = readSaturnS8(townDispatch + 1);

    if (g_target.townOverlayIndex < 0) {
        lagi::platform::logging::writef(
            "[DirectBoot] invalid town overlay index %d for game status 0x04\n",
            static_cast<int>(g_target.townOverlayIndex));
        return false;
    }

    const sSaturnPtr overlayNameEA =
        gCommonFile->getSaturnPtr(
            0x002165D8u +
            static_cast<u32>(g_target.townOverlayIndex) * 16u);

    const sSaturnPtr overlayName = readSaturnEA(overlayNameEA);
    g_target.overlayFile = readSaturnString(overlayName);
    g_target.resolved = !g_target.overlayFile.empty();

    if (!g_target.resolved) {
        lagi::platform::logging::writef(
            "[DirectBoot] failed to resolve town overlay filename\n");
        return false;
    }

    g_target.overlayPresent =
        lagi::disc::has_file(g_target.overlayFile.c_str());

    lagi::platform::logging::writef(
        "[DirectBoot] GS 0x%02X -> mode %u entry 0x%02X -> town pair (%d,%d) -> %s [%s]\n",
        static_cast<unsigned>(g_target.gameStatus),
        static_cast<unsigned>(g_target.gameMode),
        static_cast<unsigned>(g_target.gameModeEntry),
        static_cast<int>(g_target.townOverlayIndex),
        static_cast<int>(g_target.townEntryIndex),
        g_target.overlayFile.c_str(),
        g_target.overlayPresent ? "present" : "MISSING");

    char status[78];
    std::snprintf(
        status, sizeof(status),
        "[PASS] DIRECT BOOT GS04 -> %.38s",
        g_target.overlayFile.c_str());

    if (g_target.overlayPresent) {
        lagi::platform::renderer::status(status, 0xFF70E0A0u);
        return true;
    }

    lagi::platform::renderer::failure(
        "[FAIL] DIRECT BOOT TARGET OVERLAY MISSING");
    return false;
}

} // namespace lagi::azel
