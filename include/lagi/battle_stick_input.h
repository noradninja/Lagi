#pragma once

namespace lagi::input_bridge {
// Upstream's SDL pad bridge supplies digital directions above roughly half
// travel (16000/32768). Do this in every game mode so the left stick can be
// used anywhere Azel expects a Saturn D-pad. Axes retain Vita's
// right/down-positive sign.
inline unsigned short stick_directions(float x, float y)
{
    unsigned short bits = 0;
    constexpr float threshold = 16000.0f / 32768.0f;
    if (y < -threshold) bits |= 0x0010;
    if (y > threshold) bits |= 0x0020;
    if (x < -threshold) bits |= 0x0040;
    if (x > threshold) bits |= 0x0080;
    return bits;
}
}
