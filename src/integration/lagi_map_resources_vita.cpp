#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"

LagiAzelMapResources::~LagiAzelMapResources()
{
    restore();
}

bool LagiAzelMapResources::load(const LagiAzelMapDescriptor& descriptor)
{
    if(active_ || !descriptor.mapFile || !descriptor.patternFile) return false;
    const std::uint32_t graphicsBytes=getFileSize(descriptor.mapFile);
    const std::uint32_t patternBytes=getFileSize(descriptor.patternFile);
    const std::uint32_t destination=graphicsBytes>0x4000 ? 0x69000 : 0x6C000;
    if(!graphicsBytes || !patternBytes || graphicsBytes>0x80000-destination ||
        patternBytes>patterns_.size()) return false;

    // Capture both spans before the first native file write. Large SCBs may
    // overlap the pattern planes, so neither backup may be taken after load.
    const auto* graphics=getVdp2Vram(destination);
    graphicsBackup_.assign(graphics,graphics+graphicsBytes);
    std::memcpy(planeBackup_.data(),getVdp2Vram(0x70000),planeBackup_.size());
    const auto previousPatterns=patterns_;
    graphicsOffset_=destination;
    active_=true;
    std::uint32_t loadedOffset=0;
    if(!lagiAzelLoadFieldMapResources(descriptor,patterns_.data(),patterns_.size(),loadedOffset) ||
        loadedOffset!=destination) {
        restore();
        patterns_=previousPatterns;
        return false;
    }
    ready_=true;
    return true;
}

void LagiAzelMapResources::restore()
{
    ready_=false;
    if(!active_) return;
    // Restore graphics and pattern planes before the parent reestablishes
    // its menu backing. Both backups contain the same pre-load bytes where
    // they overlap. A second restore/destructor call is deliberately inert.
    std::memcpy(getVdp2Vram(graphicsOffset_),graphicsBackup_.data(),graphicsBackup_.size());
    std::memcpy(getVdp2Vram(0x70000),planeBackup_.data(),planeBackup_.size());
    graphicsOffset_=0;
    active_=false;
    std::vector<std::uint8_t>().swap(graphicsBackup_);
}
