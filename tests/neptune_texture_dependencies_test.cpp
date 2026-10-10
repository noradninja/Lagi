#include "../src/platform/vita/neptune_texture_dependencies.h"
#include "../include/lagi/debug_mesh.h"
#include <cassert>
#include <cstdio>
#include <thread>
using namespace lagi::platform::renderer;

template<class Writes, unsigned Bytes, unsigned Base>
void testWrites() {
    Writes writes;
    assert(Writes::empty(writes.consume()));
    unsigned seed = 31;
    for (unsigned iteration = 0; iteration < 200; ++iteration) {
        seed = seed * 1664525u + 1013904223u;
        unsigned start = seed % Bytes;
        unsigned size = 1u + ((seed >> 16) % 2048u);
        writes.publish(start + ((iteration & 1u) ? Base : 0u), size);
        auto snapshot = writes.consume();
        for (unsigned byte = 0; byte < size; ++byte)
            assert(Writes::overlaps(snapshot, (start + byte) % Bytes, 1u));
        assert(Writes::empty(writes.consume()));
    }
    for (unsigned start : {Bytes, 0xffffffffu}) {
        writes.publish(start, 7);
        auto snapshot = writes.consume();
        assert(Writes::overlaps(snapshot, 0, Bytes));
        assert(Writes::overlaps(snapshot, Bytes - 1, 1));
    }
    writes.publish(0, 0);
    assert(Writes::overlaps(writes.consume(), Bytes - 1, 1));
    // Concurrent producers must retain disjoint blocks without lost updates.
    std::thread a([&] { for(unsigned i=0;i<1000;++i) writes.publish(0,1); });
    std::thread b([&] { for(unsigned i=0;i<1000;++i) writes.publish(Bytes-1,1); });
    a.join(); b.join();
    auto snapshot = writes.consume();
    assert(Writes::overlaps(snapshot,0,1));
    assert(Writes::overlaps(snapshot,Bytes-1,1));
}
int main() {
    testWrites<Vdp1WriteBlocks,0x80000,0x25C00000>();
    testWrites<CramWriteBlocks,0x1000,0x25F00000>();
    lagi::azel::DecodedMode1Texture texture;
    texture.nativeDependenciesKnown=true;
    texture.width=32; texture.height=16; texture.cmdSize=0x410;
    texture.cmdSrca=0x1000; texture.cmdColr=0x200;
    Vdp1WriteBlocks pixels;
    CramWriteBlocks palettes;
    for(unsigned mode=0;mode<6;++mode) {
        texture.cmdPmod=mode<<3;
        pixels.publish(0x8000,1);
        assert(textureDependsOnWrites(texture,pixels.consume(),{},true,false));
        pixels.publish(0x70000,1);
        assert(!textureDependsOnWrites(texture,pixels.consume(),{},true,false));
        palettes.publish(0x400,1);
        assert(textureDependsOnWrites(texture,{},palettes.consume(),false,true)==(mode!=5));
        palettes.publish(0xc00,1);
        assert(textureDependsOnWrites(texture,{},palettes.consume(),false,true)==(mode==1));
    }
    texture.cmdPmod=1<<3;
    pixels.publish(0x1000,1); // LUT, separate from image at 0x8000.
    assert(textureDependsOnWrites(texture,pixels.consume(),{},true,false));
    assert(textureDependsOnWrites(texture,{},{},true,false)); // missing range fallback
    texture.cmdSize=0;
    assert(!textureDependsOnWrites(texture,{},{},true,true)); // native flat color
    texture.nativeDependenciesKnown=false;
    assert(textureDependsOnWrites(texture,{},{},true,true)); // unknown producer
    std::puts("PASS: normalized/wrapped ranges, concurrent producers, image/LUT/palette dependencies and conservative fallbacks");
}
