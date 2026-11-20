#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_display.h"
#include "lagi/prepared_disc_files.h"
#include "commonOverlay.h"

namespace lagi::worldmap {
bool prepare_map_travel(const MapDestination& destination)
{
    if (!destination.canTravel()) return false;
    std::vector<std::string> files;
    // WORLD camera state 7 only calls preparation for a positive signed +4.
    if (destination.preloadIndex > 0) {
        const auto index = unsigned(destination.preloadIndex);
        if (destination.type == 0) {
            // MAIN 06011E0A: native 20-byte field definition, font first.
            const auto definition = gCommonFile->getSaturnPtr(0x208C58 + index * 20);
            for (unsigned offset : {8u, 4u}) {
                const auto name = readSaturnEA(definition + offset);
                if (!name.isNull()) files.push_back(readSaturnString(name));
            }
        } else {
            // MAIN 06030100: resolve the town pair before the 16-byte record.
            const auto overlay = readSaturnS8(gCommonFile->getSaturnPtr(0x2166E4 + index * 2));
            if (overlay < 0) return false;
            const auto name = readSaturnEA(gCommonFile->getSaturnPtr(0x2165D8 + unsigned(overlay) * 16));
            if (!name.isNull()) files.push_back(readSaturnString(name));
        }
    }
    // MAIN 060100F2 queues the native requests. The Vita file service is
    // synchronous; retain their bytes until the ordinary loaders consume them.
    // Never overwrite Saturn 06054000 while WORLD still owns its program.
    return lagi::disc::prepare_native_files(files);
}
}
