#include "../src/platform/vita/neptune_texture_pool.h"
#include <cassert>
#include <vector>
#include <cstdio>

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
    std::puts("PASS: texture slab reuse, exact fits, exhausted tails and invalid bounds");
}
