#pragma once
#include <cstdint>
#include <algorithm>
#include <limits>
namespace lagi {
// Platform VBlank-time service. Native save time is expressed in 60 Hz ticks,
// independent of the number of task/render iterations per second.
struct PlayTimeClock {
    std::uint64_t previousUs=0, fraction=0;
    bool valid=false;
    std::uint32_t sample(std::uint64_t nowUs){
        if(!valid || nowUs<previousUs){previousUs=nowUs;fraction=0;valid=true;return 0;}
        const auto elapsed=nowUs-previousUs;previousUs=nowUs;
        const auto scaled=(elapsed%1000000)*60+fraction;
        const auto ticks=(elapsed/1000000)*60+scaled/1000000;
        fraction=scaled%1000000;
        return static_cast<std::uint32_t>(std::min<std::uint64_t>(ticks,std::numeric_limits<std::uint32_t>::max()));
    }
};
}
