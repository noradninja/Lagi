#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace neptune {

// Producer tokens identify a resource namespace for its entire lifetime.
// Resource IDs and revisions are opaque, nonzero and never reused for
// different content within that namespace. A pointer alone is not an ID.
struct GeometryResourceKey {
    std::uint64_t producer = 0;
    std::uint64_t resource = 0;
    std::uint64_t revision = 0;

    constexpr bool valid() const noexcept {
        return producer != 0 && resource != 0 && revision != 0;
    }
    constexpr bool operator==(const GeometryResourceKey& other) const noexcept {
        return producer == other.producer && resource == other.resource &&
            revision == other.revision;
    }
};

enum class GeometrySpace : std::uint8_t { Local, Transformed };

// Identity of the actual cached bytes, not an instance or a draw call.
// Layout and coordinateConvention are nonzero renderer-defined versioned
// identifiers. bakedAttributes must change whenever material-dependent UVs
// or any other baked attributes change; zero means no such dependency.
// Lighting, raster state and visibility are excluded only if not baked.
struct GeometryRepresentationKey {
    GeometryResourceKey source{};
    std::uint32_t layout = 0;
    std::uint32_t coordinateConvention = 0;
    std::uint64_t bakedAttributes = 0;
    GeometrySpace space = GeometrySpace::Local;
    std::array<std::int32_t, 12> transform{};

    constexpr bool valid() const noexcept {
        if (!source.valid() || !layout || !coordinateConvention)
            return false;
        if (space == GeometrySpace::Transformed)
            return true;
        if (space != GeometrySpace::Local)
            return false;
        for (auto value : transform)
            if (value != 0) return false;
        return true;
    }
    bool operator==(const GeometryRepresentationKey& other) const noexcept {
        return source == other.source && layout == other.layout &&
            coordinateConvention == other.coordinateConvention &&
            bakedAttributes == other.bakedAttributes &&
            space == other.space && transform == other.transform;
    }
};

// Hash is a lookup accelerator only; full equality decides reuse.
struct GeometryRepresentationKeyHash {
    std::size_t operator()(const GeometryRepresentationKey& key) const noexcept {
        std::uint64_t h = 14695981039346656037ull;
        const auto mix = [&h](std::uint64_t value) {
            h ^= value;
            h *= 1099511628211ull;
        };
        mix(key.source.producer);
        mix(key.source.resource);
        mix(key.source.revision);
        mix(key.layout);
        mix(key.coordinateConvention);
        mix(key.bakedAttributes);
        mix(static_cast<std::uint8_t>(key.space));
        for (auto value : key.transform)
            mix(static_cast<std::uint32_t>(value));
        return static_cast<std::size_t>(h ^ (h >> 32));
    }
};

} // namespace neptune
