#include "lagi/playback_power_tick.h"
#include <cassert>
int main(){
    lagi::PlaybackPowerTick tick;
    assert(tick.due(0));assert(!tick.due(0));assert(!tick.due(999999));
    assert(tick.due(1000000));assert(!tick.due(1000001));
    assert(tick.due(9000000)); // long gap: one service, not catch-up ticks
    assert(!tick.due(9000000));
    assert(tick.due(2)); // clock restart cannot suppress service indefinitely
    assert(!tick.due(3));
}
