#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace lagi::battle::effects {
// BTL_A3 0608CAC0..0608CC4C, native motion-record stride 0x24.
// Values are raw signed 16.16; angles index Azel's native trig tables.
struct Motion {
    std::array<std::int32_t, 3> acceleration{};
    std::array<std::int32_t, 3> velocity{};
    std::array<std::int32_t, 3> angles{};
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
    value.angles[1] = multiply(sine(angle()), firstMagnitude);
    const auto secondMagnitude = magnitude();
    const auto angularRadius = multiply(cosine(angle()), secondMagnitude);
    value.angles[2] = multiply(sine(angle()), angularRadius);
    value.angles[0] = multiply(cosine(angle()), angularRadius);
    return value;
}

// First 0x24 bytes of the separate native 0x38 particle record. Unlike the
// effect's angular Motion above, this contains a physical particle position.
struct ParticleMotion {
    std::array<std::int32_t, 3> acceleration{};
    std::array<std::int32_t, 3> velocity{};
    std::array<std::int32_t, 3> position{};
};
static_assert(sizeof(ParticleMotion) == 0x24);
inline std::int32_t add(std::int32_t a, std::int32_t b)
{
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
}
inline std::int32_t subtract(std::int32_t a, std::int32_t b)
{
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
}

// Motion portion of BTL_A3 0608D60C..0608D71E. The caller must still advance
// its animated quad, even when state != 1 (the jump at 0608D622 goes to that
// shared animation tail). Do not infer the sign of the spring from its name:
// native MOV.W sign-extends FD71 to -655; the damping constant is 0CCC.
inline void advanceParticleMotion(ParticleMotion& particle,
    const std::array<std::int32_t,3>& target, std::uint16_t state)
{
    if (state != 1)
        return;
    for (unsigned axis=0; axis<3; ++axis) {
        particle.acceleration[axis] = add(particle.acceleration[axis],
            multiply(subtract(target[axis],particle.position[axis]), -655));
        particle.velocity[axis] = add(particle.velocity[axis], particle.acceleration[axis]);
        particle.velocity[axis] = subtract(particle.velocity[axis],
            multiply(particle.velocity[axis],0x0CCC));
        particle.position[axis] = add(particle.position[axis],particle.velocity[axis]);
        particle.acceleration[axis] = 0;
    }
}
} // namespace lagi::battle::effects
