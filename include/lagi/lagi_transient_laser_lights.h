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

// Keep the brightest native channel unchanged while expanding the hue's
// chroma. A neutral input remains neutral. Used for effect lights only; the
// authored sprite/glyph pixels and the point-light falloff are untouched.
inline constexpr std::uint32_t saturate_rgb888_peak(std::uint32_t color) {
    const std::uint32_t r = (color >> 16) & 0xFFu;
    const std::uint32_t g = (color >> 8) & 0xFFu;
    const std::uint32_t b = color & 0xFFu;
    const std::uint32_t low = r < g ? (r < b ? r : b) : (g < b ? g : b);
    const std::uint32_t high = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (low == high) return color & 0xFFFFFFu;
    const auto spread = [=](std::uint32_t channel) {
        return (channel - low) * high / (high - low);
    };
    return (spread(r) << 16) | (spread(g) << 8) | spread(b);
}

// VDP1 Gouraud entries are offsets, not a finished texture colour. Their
// channel ordering still identifies the visible beam's chromatic direction.
// Map that direction to the existing light peak, avoiding a texture-dependent
// brightness jump while following the active native gradient.
inline constexpr std::uint32_t gouraud555_hue_at_peak(
    std::uint16_t gouraud, std::uint32_t peak) {
    const std::uint32_t r = gouraud & 31u;
    const std::uint32_t g = (gouraud >> 5) & 31u;
    const std::uint32_t b = (gouraud >> 10) & 31u;
    const std::uint32_t low = r < g ? (r < b ? r : b) : (g < b ? g : b);
    const std::uint32_t high = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (low == high) return (peak << 16) | (peak << 8) | peak;
    const auto channel = [=](std::uint32_t value) {
        return (value - low) * peak / (high - low);
    };
    return (channel(r) << 16) | (channel(g) << 8) | channel(b);
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
