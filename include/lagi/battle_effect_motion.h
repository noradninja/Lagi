#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <span>

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

// Logical state, not a binary overlay: native animated-quad storage occupies
// +24..2B and will be owned by the producer, separately from this initializer.
struct ParticleSeed {
    ParticleMotion motion;
    std::int32_t field2C=0;
    std::uint32_t type=0;
    std::uint16_t delay=0;
    std::uint16_t state=0;
};

// BTL_A3 0608D5A0..0608D5FC plus 0608CC8A..0608CD2C.
// The native type multiplier really is count-1, not count. Keep that authored
// choice and seven random draws, including the field not yet interpreted.
template<class Random, class Sine, class Cosine>
ParticleSeed initializeParticleSeed(std::uint32_t typeCount,
    std::int32_t radius, Random&& random, Sine&& sine, Cosine&& cosine)
{
    auto sample=[&](std::int32_t range){
        return multiply(static_cast<std::uint32_t>(random()) >> 16,range);
    };
    auto angle=[&](){return static_cast<std::uint32_t>(random()) >> 20;};
    ParticleSeed particle;
    particle.type=sample(static_cast<std::int32_t>(typeCount)-1);
    particle.delay=static_cast<std::uint16_t>(sample(50));
    particle.field2C=-sample(164)-40;
    particle.motion.position[1]=multiply(sine(angle()),radius);
    const auto horizontal=multiply(cosine(angle()),radius);
    particle.motion.position[2]=multiply(sine(angle()),horizontal);
    particle.motion.position[0]=multiply(cosine(angle()),horizontal);
    return particle;
}
inline std::int32_t add(std::int32_t a, std::int32_t b)
{
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
}
inline std::int32_t subtract(std::int32_t a, std::int32_t b)
{
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
}

inline std::int32_t angleDifference(std::int32_t value)
{
    const auto bits = static_cast<std::uint32_t>(value);
    return std::bit_cast<std::int32_t>((bits & 0x08000000u)
        ? bits | 0xF0000000u : bits & 0x0FFFFFFFu);
}

// Parent effect motion loop, BTL_A3 0608CDA8..0608D40C. Native iteration is
// in-place: later records observe their already-updated predecessor, and the
// final record observes the updated first record. Do not snapshot the ring.
template<class Random, class Sine, class Cosine>
void advanceMotionRing(std::span<Motion> records,
    Random&& random, Sine&& sine, Cosine&& cosine)
{
    for (std::size_t i=0; i<records.size(); ++i) {
        auto& current=records[i];
        const auto previous=i ? i-1 : records.size()-1;
        const auto next=i+1==records.size() ? 0 : i+1;
        for (const auto neighbor : {previous,next}) {
            for (unsigned axis=0; axis<3; ++axis) {
                const auto delta=angleDifference(subtract(records[neighbor].angles[axis],current.angles[axis]));
                current.acceleration[axis]=add(current.acceleration[axis],multiply(delta,0x028F));
            }
        }
        // The decision consumes one RNG draw per record, not one per frame.
        if ((static_cast<std::uint32_t>(random()) & 0x1Fu)==0) {
            auto angle=[&](){return static_cast<std::uint32_t>(random()) >> 20;};
            auto magnitude=[&](){return multiply(static_cast<std::uint32_t>(random()) >> 16,0x00012345);};
            std::array<std::int32_t,3> impulse{};
            const auto firstMagnitude=magnitude();
            impulse[1]=multiply(sine(angle()),firstMagnitude);
            const auto secondMagnitude=magnitude();
            const auto radius=multiply(cosine(angle()),secondMagnitude);
            impulse[2]=multiply(sine(angle()),radius);
            impulse[0]=multiply(cosine(angle()),radius);
            for (unsigned axis=0; axis<3; ++axis)
                current.acceleration[axis]=add(current.acceleration[axis],impulse[axis]);
        }
        for (unsigned axis=0; axis<3; ++axis) {
            current.velocity[axis]=add(current.velocity[axis],current.acceleration[axis]);
            current.velocity[axis]=subtract(current.velocity[axis],multiply(current.velocity[axis],0x03D7));
            current.angles[axis]=add(current.angles[axis],current.velocity[axis]) & 0x0FFFFFFF;
            current.acceleration[axis]=0;
        }
    }
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

// Logical motion state for a status-cure sparkle (native record stride 34h,
// animated quad +00, these six fields +08..1C, position +20, scale +2C).
struct SparkleMotion {
    std::int32_t radialAcceleration=0, radialVelocity=0, radius=0;
    std::int32_t angularAcceleration=0, angularVelocity=0, angle=0;
    std::int32_t scale=0;
};

// BTL_A3 06099D10..06099D4C. Angular damping really is zero in this
// routine; radial damping is 051E. Neither acceleration is cleared here.
inline void advanceSparkleMotion(SparkleMotion& sparkle)
{
    sparkle.angularVelocity=add(sparkle.angularVelocity,sparkle.angularAcceleration);
    sparkle.angle=add(sparkle.angle,sparkle.angularVelocity) & 0x0FFFFFFF;
    sparkle.radialVelocity=add(sparkle.radialVelocity,sparkle.radialAcceleration);
    sparkle.radialVelocity=subtract(sparkle.radialVelocity,multiply(sparkle.radialVelocity,0x051E));
    sparkle.radius=add(sparkle.radius,sparkle.radialVelocity);
}

// Native 06099D4E..06099D7A advances the quad first. Only its end-of-cycle
// flag consumes RNG, decreases scale and invokes 06099E14 to reseed motion.
template<class Random, class Reseed>
void finishSparkleAnimationStep(SparkleMotion& sparkle,unsigned animationFlags,
    Random&& random, Reseed&& reseed)
{
    if (!(animationFlags & 2u))
        return;
    const auto decay=multiply(static_cast<std::uint32_t>(random()) >> 16,0x3334)+0x0CCC;
    sparkle.scale=subtract(sparkle.scale,decay);
    reseed();
}
} // namespace lagi::battle::effects
