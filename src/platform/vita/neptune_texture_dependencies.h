#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace lagi::platform::renderer {

// Bounded, allocation-free producer notifications. Block granularity can
// over-invalidate but never intentionally excludes a written byte.
template<unsigned Bytes, unsigned BlockBytes, unsigned Base>
class TextureWriteBlocks {
public:
    static constexpr unsigned Blocks = Bytes / BlockBytes;
    using Snapshot = std::array<std::uint32_t, (Blocks + 31u) / 32u>;
    void publish(unsigned start, unsigned size) {
        if (start >= Base && start - Base < Bytes) start -= Base;
        if (start >= Bytes || size == 0u || size >= Bytes) {
            for (auto& word : pending_) word.fetch_or(~0u, std::memory_order_release);
            return;
        }
        const unsigned firstSize = size < Bytes - start ? size : Bytes - start;
        mark(start, firstSize);
        if (size > firstSize) mark(0u, size - firstSize);
    }
    Snapshot consume() {
        Snapshot result{};
        for (unsigned i = 0; i < result.size(); ++i)
            result[i] = pending_[i].exchange(0u, std::memory_order_acquire);
        return result;
    }
    static bool empty(const Snapshot& writes) {
        for (auto word : writes) if (word) return false;
        return true;
    }
    static bool overlaps(const Snapshot& writes, unsigned start, unsigned size) {
        if (!size) return false;
        if (start >= Bytes || size >= Bytes) return !empty(writes);
        const unsigned firstSize = size < Bytes - start ? size : Bytes - start;
        if (overlapsLinear(writes, start, firstSize)) return true;
        return size > firstSize && overlapsLinear(writes, 0u, size - firstSize);
    }
private:
    std::array<std::atomic<std::uint32_t>, (Blocks + 31u) / 32u> pending_{};
    void mark(unsigned start, unsigned size) {
        for (unsigned block = start / BlockBytes;
             block <= (start + size - 1u) / BlockBytes; ++block)
            pending_[block / 32u].fetch_or(1u << (block % 32u), std::memory_order_release);
    }
    static bool overlapsLinear(const Snapshot& writes, unsigned start, unsigned size) {
        for (unsigned block = start / BlockBytes;
             block <= (start + size - 1u) / BlockBytes; ++block)
            if (writes[block / 32u] & (1u << (block % 32u))) return true;
        return false;
    }
};
using Vdp1WriteBlocks = TextureWriteBlocks<0x80000u, 512u, 0x25C00000u>;
using CramWriteBlocks = TextureWriteBlocks<0x1000u, 32u, 0x25F00000u>;

template<class Texture>
bool textureDependsOnWrites(const Texture& texture,
    const Vdp1WriteBlocks::Snapshot& pixels,
    const CramWriteBlocks::Snapshot& palettes,
    bool pixelNotification, bool paletteNotification)
{
    if (!texture.nativeDependenciesKnown) return pixelNotification || paletteNotification;
    // Native flat polygons have CMDSIZE=0 and no image/palette dependency.
    if (texture.cmdSize == 0u) return false;
    const unsigned mode = (texture.cmdPmod >> 3u) & 7u;
    if (mode > 5u) return pixelNotification || paletteNotification;
    if (pixelNotification) {
        // Empty after a flag can mean a concurrently consumed notification.
        // Keep the legacy full invalidation rather than trusting missing data.
        if (Vdp1WriteBlocks::empty(pixels)) return true;
        const unsigned bytes = texture.width * texture.height * (mode == 5u ? 2u : 1u);
        if (Vdp1WriteBlocks::overlaps(pixels, unsigned(texture.cmdSrca) << 3u,
                mode <= 1u ? bytes / 2u : bytes)) return true;
        if (mode == 1u && Vdp1WriteBlocks::overlaps(pixels,
                unsigned(texture.cmdColr) << 3u, 32u)) return true;
    }
    if (!paletteNotification || mode == 5u) return false;
    if (CramWriteBlocks::empty(palettes)) return true;
    // Captured reads include zero colors and eager palette entries. LUT RGB555
    // entries do not access CRAM; palette-indexed LUT entries can access any
    // block, not necessarily the material's nominal bank.
    for (unsigned word = 0; word < palettes.size(); ++word)
        if (texture.nativeCramDependencies[word] & palettes[word]) return true;
    return false;
}
} // namespace lagi::platform::renderer
