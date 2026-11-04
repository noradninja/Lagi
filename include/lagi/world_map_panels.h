#pragma once
#include <cstdint>
#include <span>

namespace lagi::worldmap {
// WORLD 06055772..06055812. Native mainGameState.bitField bytes, not input.
inline int map_panel_variant(std::span<const std::uint8_t> story, unsigned panel)
{
    if(story.size()<16 || panel>1) return -1;
    if(panel==0) {
        if(story[9]&0x10) return -1;
        if(story[9]&0x40) return 4;
        if(story[12]&0x40) return 3;
        if(story[8]&0x40) return 2;
        if(story[7]&0x10) return 1;
        if(story[5]&0x01) return 0;
    } else {
        if(story[12]&0x40) return -1;
        if(story[15]&0x01) return 3;
        if(story[11]&0x04) return 2;
        if(story[5]&0x40) return 1;
        if(story[4]&0x08) return 0;
    }
    return -1;
}
}
