#include "neptune/resident_geometry_cache.h"
#include <array>
#include <cassert>
#include <cstdio>

int main() {
    neptune::GeometryArena<8> arena(512);
    neptune::ResidentGeometryCache<8,4> cache;
    std::array<unsigned char,512> storage{};
    auto renderer=arena.allocate(64,64);
    assert(renderer);
    auto owner=std::make_shared<const int>(1);
    neptune::GeometryRepresentationKey key{};
    key.source={1,2,1};
    key.layout=2;
    key.coordinateConvention=1;
    std::array<unsigned char,64> first{},changed{};
    first.fill(0x41); changed.fill(0x52);
    auto a=cache.acquire(arena,storage.data(),key,owner,first.data(),first.size(),1);
    assert(a && a.offset==64);
    auto again=cache.acquire(arena,storage.data(),key,owner,first.data(),first.size(),1);
    assert(again.id==a.id && cache.stats().hits==1);
    auto b=cache.acquire(arena,storage.data(),key,owner,changed.data(),changed.size(),1);
    assert(b && b.offset!=a.offset && cache.stats().mismatches==1);
    assert(storage[a.offset]==0x41 && storage[b.offset]==0x52);
    assert(arena.used()==192 && arena.completeThrough(1) && arena.used()==128);
    auto replacement=std::make_shared<const int>(2);
    auto c=cache.acquire(arena,storage.data(),key,replacement,changed.data(),changed.size(),2);
    assert(c && c.id!=b.id && storage[c.offset]==0x52);
    assert(arena.completeThrough(2));
    replacement.reset();
    cache.sweep(arena,3);
    assert(cache.size()==0 && arena.used()==renderer.bytes);
    auto invalid=key; invalid.source={};
    assert(!cache.acquire(arena,storage.data(),invalid,owner,first.data(),first.size(),3));
    assert(!cache.acquire(arena,storage.data(),key,owner,first.data(),first.size(),0));
    auto pressure=arena.allocate(448,64);
    assert(pressure && !cache.acquire(arena,storage.data(),key,owner,first.data(),first.size(),3));
    assert(arena.retire(pressure));
    auto d=cache.acquire(arena,storage.data(),key,owner,first.data(),first.size(),3);
    assert(d);
    cache.sweep(arena,3,0);
    assert(cache.size()==1); // Current transaction pinned.
    cache.sweep(arena,4,0);
    assert(cache.size()==0 && arena.used()==128); // Still in flight.
    assert(arena.completeThrough(3) && arena.used()==64);
    std::puts("PASS Neptune immutable resident geometry cache");
}
