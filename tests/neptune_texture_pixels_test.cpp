#include "../src/platform/vita/neptune_texture_pixels.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>

int main()
{
    using lagi::platform::renderer::texturePixelsOpaque;
    assert(texturePixelsOpaque(nullptr, 0));
    unsigned cases = 0;
    for (unsigned length = 0; length <= 257; ++length) {
        std::vector<std::uint32_t> pixels(length, 0xffffffffu);
        assert(texturePixelsOpaque(pixels.data(), pixels.size()));
        for (unsigned position = 0; position < length; ++position) {
            for (unsigned alpha = 0; alpha < 256; ++alpha) {
                pixels[position] = (alpha << 24) | 0x123456u;
                const bool reference = std::all_of(pixels.begin(), pixels.end(),
                    [](std::uint32_t p) { return (p >> 24) >= 128; });
                assert(texturePixelsOpaque(pixels.data(), pixels.size()) == reference);
                ++cases;
            }
            pixels[position] = 0xffffffffu;
        }
    }
    std::printf("PASS: %u opacity comparisons, all alpha values, positions and short tails\n", cases);
}
