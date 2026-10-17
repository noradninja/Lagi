#pragma once
#include <array>
#include <cstdint>

namespace lagi::battle::effects {
// BTL_A3 0608CAC0..0608CC4C, native motion-record stride 0x24.
// Values are raw signed 16.16; angles index Azel's native trig tables.
struct Motion {
    std::array<std::int32_t, 3> position{};
    std::array<std::int32_t, 3> velocity{};
    std::array<std::int32_t, 3> angularVelocity{};
};
static_assert(sizeof(Motion) == 0x24);

inline std::int32_t multiply(std::int32_t a, std::int32_t b)
{
    return static_cast<std::int32_t>((static_cast<std::int64_t>(a) * b) >> 16);
}

// Random, sine and cosine are supplied by Azel. Preserve the ten draws and
// their order: the native routine uses independent angles, not a replacement
// spherical distribution. No game table or copyrighted data is embedded.
template<class Random, class Sine, class Cosine>
Motion initializeMotion(Random&& random, Sine&& sine, Cosine&& cosine)
{
    auto angle = [&]() { return static_cast<std::uint32_t>(random()) >> 20; };
    auto magnitude = [&]() {
        // MTH_Mul(randomNumber() >> 16, 04000000), without rounding.
        return static_cast<std::int32_t>((static_cast<std::uint32_t>(random()) >> 16) * 1024u);
    };
    Motion value;
    constexpr std::int32_t speed = 0x00AAAAAA;
    value.velocity[1] = multiply(sine(angle()), speed);
    const auto radius = multiply(cosine(angle()), speed);
    value.velocity[2] = multiply(sine(angle()), radius);
    value.velocity[0] = multiply(cosine(angle()), radius);
    const auto firstMagnitude = magnitude();
    value.angularVelocity[1] = multiply(sine(angle()), firstMagnitude);
    const auto secondMagnitude = magnitude();
    const auto angularRadius = multiply(cosine(angle()), secondMagnitude);
    value.angularVelocity[2] = multiply(sine(angle()), angularRadius);
    value.angularVelocity[0] = multiply(cosine(angle()), angularRadius);
    return value;
}
} // namespace lagi::battle::effects
