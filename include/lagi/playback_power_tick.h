#pragma once
#include <cstdint>
namespace lagi {
// Wall-clock service only; never advances a simulation or playback clock.
struct PlaybackPowerTick {
    bool started=false;
    std::uint64_t last=0;
    bool due(std::uint64_t now) {
        if(started && now>=last && now-last<1000000u)return false;
        started=true;last=now;return true;
    }
};
}
