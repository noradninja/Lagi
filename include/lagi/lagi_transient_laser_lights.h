#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace lagi {

// Hard cap shared by transient field and battle laser lighting. This is a
// producer-side payload only: consumers may choose fewer, never more.
inline constexpr std::size_t kMaxTransientLaserLights = 4;
inline constexpr std::uint32_t kBattleTransientLightDivisor = 4u;

inline constexpr std::uint32_t quarter_rgb888(std::uint32_t packedColor) {
    const auto quarter = [](std::uint32_t channel) {
        return channel / kBattleTransientLightDivisor;
    };
    return (quarter((packedColor >> 16) & 0xFFu) << 16) |
        (quarter((packedColor >> 8) & 0xFFu) << 8) |
        quarter(packedColor & 0xFFu);
}

struct TransientLaserLight {
    std::uint64_t source = 0;       // Native task identity for same-frame updates
    std::int32_t worldPosition[3]{}; // Signed 16.16, original Azel world space
    std::uint16_t color[3]{};       // Saturn packed channel order: B,G,R
    std::uint32_t range = 0;        // Original native point-light range value
};

struct TransientLaserLightFrame {
    std::array<TransientLaserLight, kMaxTransientLaserLights> lights{};
    std::size_t count = 0;

    void clear() { count = 0; }

    // Existing sources update their own light. Once four sources are present,
    // later sources are ignored deterministically; no heap or replacement
    // policy can alter the active beam set mid-frame.
    bool upsert(const TransientLaserLight& light) {
        if (!light.source || !light.range) return false;
        for (std::size_t i = 0; i < count; ++i) {
            if (lights[i].source == light.source) {
                lights[i] = light;
                return true;
            }
        }
        if (count == kMaxTransientLaserLights) return false;
        lights[count++] = light;
        return true;
    }
};
} // namespace lagi
