#pragma once

namespace lagi::input_bridge {
// Upstream's SDL pad bridge also supplies digital directions above roughly
// half travel (16000/32768). Axes here retain Vita's right/down-positive sign.
inline unsigned short battle_stick_directions(float x, float y, bool battle)
{
    if (!battle) return 0;
    unsigned short bits = 0;
    constexpr float threshold = 16000.0f / 32768.0f;
    if (y < -threshold) bits |= 0x0010;
    if (y > threshold) bits |= 0x0020;
    if (x < -threshold) bits |= 0x0040;
    if (x > threshold) bits |= 0x0080;
    return bits;
}
}
