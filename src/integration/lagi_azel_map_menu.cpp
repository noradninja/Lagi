#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"

bool lagiAzelLoadFieldMapResources(const LagiAzelMapDescriptor& descriptor,
    std::uint8_t* patterns, std::size_t capacity, std::uint32_t& graphicsVramOffset)
{
    graphicsVramOffset = 0;
    if (!descriptor.mapFile || !descriptor.patternFile || !patterns) return false;
    const std::uint32_t graphicsBytes = getFileSize(descriptor.mapFile);
    const std::uint32_t patternBytes = getFileSize(descriptor.patternFile);
    // Native 06016E84: strictly greater than 4000 uses the lower destination.
    const std::uint32_t destination = graphicsBytes > 0x4000 ? 0x69000 : 0x6C000;
    if (!graphicsBytes || !patternBytes || graphicsBytes > 0x80000 - destination ||
        patternBytes > capacity || (patternBytes <= 0x800 && capacity < 0x800)) return false;
    if (loadFile2(descriptor.mapFile,getVdp2Vram(destination),0) < 0) return false;
    // 06016EEE: for a short/equal 800-byte PNB, initialize exactly 800 bytes
    // with big-endian 0238 pattern words before the native file overwrites it.
    if (patternBytes <= 0x800)
        for (unsigned i = 0; i < 0x800; i += 2) {
            patterns[i] = 0x02;
            patterns[i + 1] = 0x38;
        }
    if (loadFile2(descriptor.patternFile,patterns,0) < 0) return false;
    graphicsVramOffset = destination;
    return true;
}
