#include "../src/platform/vita/neptune_texture_pool.h"
#include <cassert>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cstdint>

struct Slab { std::size_t bytes, used; };
int main()
{
    using lagi::platform::renderer::findTextureSlabWithSpace;
    std::vector<Slab> slabs;
    assert(findTextureSlabWithSpace(slabs, 64) == 0);
    slabs = {{1024, 512}, {1024, 1000}};
    assert(findTextureSlabWithSpace(slabs, 512) == 0);
    assert(findTextureSlabWithSpace(slabs, 513) == slabs.size());
    slabs[0].used += 512;
    assert(findTextureSlabWithSpace(slabs, 24) == 1);
    slabs = {{128, 129}, {256, 192}, {512, 0}};
    assert(findTextureSlabWithSpace(slabs, 64) == 1);
    assert(findTextureSlabWithSpace(slabs, 65) == 2);
    // A batch larger than the newest tail still reuses older tails per texture.
    slabs = {{1024, 256}, {1024, 1024}};
    for (int i = 0; i < 12; ++i) {
        auto index = findTextureSlabWithSpace(slabs, 64);
        assert(index == 0);
        slabs[index].used += 64;
        assert(slabs[index].used <= slabs[index].bytes);
    }
    assert(findTextureSlabWithSpace(slabs, 64) == slabs.size());
    // Exercise the renderer's growth policy with deterministic varied batches.
    // This uses the actual selector, but models allocation rather than GXM.
    struct Range { std::size_t slab, begin, end; };
    std::vector<Range> allocations;
    slabs.clear();
    std::uint32_t random = 0x173c;
    for (unsigned batch = 0; batch < 80; ++batch) {
        std::vector<std::size_t> sizes;
        std::size_t remaining = 0;
        for (unsigned i = 0; i < 25; ++i) {
            random = random * 1664525u + 1013904223u;
            const auto bytes = std::size_t(1 + (random % 65536));
            sizes.push_back((bytes + 63) & ~std::size_t(63));
            remaining += sizes.back();
        }
        for (auto bytes : sizes) {
            auto index = findTextureSlabWithSpace(slabs, bytes);
            if (index == slabs.size())
                slabs.push_back({std::max(remaining, std::size_t(1024 * 1024)), 0});
            auto& slab = slabs[index];
            const Range range{index, slab.used, slab.used + bytes};
            assert(range.begin % 64 == 0 && range.end <= slab.bytes);
            for (const auto& previous : allocations) {
                if (previous.slab == index)
                    assert(range.end <= previous.begin || range.begin >= previous.end);
            }
            allocations.push_back(range);
            slab.used = range.end;
            assert(remaining >= bytes);
            remaining -= bytes;
        }
        assert(remaining == 0);
    }
    assert(allocations.size() == 2000);
    std::puts("PASS: texture slab reuse, exact fits, exhausted tails and invalid bounds");
    std::puts("PASS: 2000 modeled allocations preserve alignment, bounds and disjoint ranges");
}
