#pragma once
#include <cstddef>
#include <cstdint>

namespace lagi::platform::renderer {

inline bool texturePixelsOpaque(const std::uint32_t* pixels, std::size_t count)
{
    // Alpha >=128 is exactly bit 31. Test eight packed pixels per branch;
    // do not read beyond the supplied span, including the final short block.
    std::size_t i = 0;
    for (; count - i >= 8; i += 8) {
        const auto opaqueBits = pixels[i] & pixels[i + 1] & pixels[i + 2] &
            pixels[i + 3] & pixels[i + 4] & pixels[i + 5] &
            pixels[i + 6] & pixels[i + 7];
        if ((opaqueBits & 0x80000000u) == 0)
            return false;
    }
    for (; i < count; ++i) {
        if ((pixels[i] & 0x80000000u) == 0)
            return false;
    }
    return true;
}

} // namespace lagi::platform::renderer
