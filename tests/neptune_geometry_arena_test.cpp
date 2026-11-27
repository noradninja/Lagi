#include "neptune/geometry_arena.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
    neptune::GeometryArena<4> arena(1024);
    assert(!arena.allocate(0,16));
    assert(!arena.allocate(1,0));
    assert(!arena.allocate(1,3));
    assert(!arena.allocate(std::numeric_limits<std::size_t>::max(),16));
    auto a=arena.allocate(100,64);
    auto b=arena.allocate(100,64);
    auto c=arena.allocate(128,256);
    assert(a && b && c);
    assert(a.offset==0 && b.offset==128 && c.offset==256);
    assert(arena.used()==328 && arena.peak()==328 && arena.largestFree()==640);
    assert(arena.markUsed(b,7));
    assert(arena.retire(b));
    assert(!arena.markUsed(b,8));
    assert(!arena.retire(b));
    assert(arena.completeThrough(6) && arena.used()==328);
    assert(!arena.completeThrough(5));
    assert(arena.completeThrough(7) && arena.used()==228);
    auto d=arena.allocate(100,64);
    assert(d && d.offset==128 && d.id!=b.id);
    assert(!arena.retire(b));
    auto invalid=d; ++invalid.bytes;
    assert(!arena.markUsed(invalid,8));
    auto e=arena.allocate(640,16);
    assert(e && e.offset==384);
    assert(!arena.allocate(1,1)); // metadata limit
    assert(arena.peak()==968);
    assert(arena.retire(a) && arena.retire(c) && arena.retire(d) && arena.retire(e));
    assert(arena.used()==0 && arena.count()==0 && arena.largestFree()==1024);
    auto whole=arena.allocate(1024,256);
    assert(whole && whole.offset==0 && arena.used()==1024);
    assert(!arena.markUsed(whole,7)); // completed serial cannot protect a range
    assert(arena.markUsed(whole,8) && arena.markUsed(whole,9));
    assert(arena.retire(whole));
    assert(arena.completeThrough(8) && arena.used()==1024);
    assert(!arena.allocate(1,1));
    assert(arena.completeThrough(9) && arena.used()==0);
    std::puts("PASS Neptune geometry arena range and retirement");
}
