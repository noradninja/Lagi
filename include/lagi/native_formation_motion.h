#pragma once
#include <cstdint>
#include <cstring>

namespace lagi {
// BTL_T0 0609DE44..0609DF72. Separate named fields: never cast a host
// work-area pointer to the Saturn byte layout (task headers differ).
struct NativeFormationSpringAxis {
    std::int32_t target;
    std::int32_t current;
    std::int32_t velocity;
    std::int32_t acceleration;
    std::int32_t world;
};
inline std::int32_t formationSignedBits(std::uint32_t value) {
    std::int32_t result;
    std::memcpy(&result, &value, sizeof result);
    return result;
}
inline std::int32_t formationAdd(std::int32_t a, std::int32_t b) {
    return formationSignedBits(std::uint32_t(a) + std::uint32_t(b));
}
inline std::int32_t formationSubtract(std::int32_t a, std::int32_t b) {
    return formationSignedBits(std::uint32_t(a) - std::uint32_t(b));
}
inline std::int32_t formationMultiply(std::int32_t a, std::int32_t b) {
    // Native DMULS product bits 16..47, including negative products and wrap.
    const auto product = std::int64_t(a) * std::int64_t(b);
    return formationSignedBits(std::uint32_t(std::uint64_t(product) >> 16));
}
inline void stepNativeFormationSpring(NativeFormationSpringAxis& axis,
                                     std::int32_t center,
                                     std::int32_t spring,
                                     std::int32_t damping) {
    const auto difference = std::uint32_t(axis.target) - std::uint32_t(axis.current);
    // SH2 TST 08000000 followed by OR F0000000 or AND 0FFFFFFF:
    // signed 28-bit, NOT a clamp or ordinary signed 32-bit subtraction.
    const auto signedDifference = formationSignedBits((difference & 0x08000000u)
        ? difference | 0xf0000000u : difference & 0x0fffffffu);
    axis.acceleration = formationAdd(axis.acceleration,
                                    formationMultiply(signedDifference, spring));
    axis.acceleration = formationSubtract(axis.acceleration,
                                         formationMultiply(axis.velocity, damping));
    axis.velocity = formationAdd(axis.velocity, axis.acceleration);
    axis.current = formationAdd(axis.current, axis.velocity);
    axis.world = formationAdd(center, axis.current);
    axis.acceleration = 0;
}
} // namespace lagi
