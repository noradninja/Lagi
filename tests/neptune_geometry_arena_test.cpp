#include "neptune/geometry_arena.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>
#include <random>

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
    // Fragmented randomized workloads, independent byte occupancy oracle.
    neptune::GeometryArena<128> stress(8192);
    using Range=neptune::GeometryArena<128>::Range;
    std::vector<Range> live;
    std::vector<bool> occupied(8192,false);
    std::mt19937 rng(0x4e455054u);
    for (unsigned step=0;step<20000;++step) {
        if (!live.empty() && (rng()%3==0 || live.size()==128)) {
            const auto index=rng()%live.size();
            const auto range=live[index];
            assert(stress.retire(range));
            for(auto i=range.offset;i<range.offset+range.bytes;++i) {
                assert(occupied[i]); occupied[i]=false;
            }
            live[index]=live.back(); live.pop_back();
        } else {
            const std::size_t bytes=1+rng()%256, alignment=std::size_t(1)<<(rng()%8);
            auto range=stress.allocate(bytes,alignment);
            if(range) {
                assert(range.offset%alignment==0 && range.bytes==bytes);
                assert(range.offset+bytes<=occupied.size());
                for(auto i=range.offset;i<range.offset+bytes;++i) {
                    assert(!occupied[i]); occupied[i]=true;
                }
                live.push_back(range);
            } else if(live.size()<128) {
                bool fits=false;
                for(std::size_t i=0;i+bytes<=occupied.size();i+=alignment) {
                    bool free=true;
                    for(auto j=i;j<i+bytes;++j) if(occupied[j]) {free=false;break;}
                    if(free) {fits=true;break;}
                }
                assert(!fits);
            }
        }
        std::size_t used=0,run=0,largest=0;
        for(bool bit:occupied) {
            if(bit) {++used;run=0;} else {++run;if(run>largest)largest=run;}
        }
        assert(stress.used()==used && stress.count()==live.size());
        assert(stress.largestFree()==largest);
    }
    for(auto range:live) assert(stress.retire(range));
    assert(stress.used()==0 && stress.largestFree()==8192);
    // Retirement ordering must follow GPU serials, not allocation order.
    auto x=stress.allocate(128,64), y=stress.allocate(128,64);
    assert(stress.markUsed(x,12) && stress.markUsed(y,10));
    assert(stress.retire(x) && stress.retire(y));
    assert(stress.completeThrough(10) && stress.used()==128);
    assert(stress.completeThrough(12) && stress.used()==0);
    // Physical domain: preserve renderer-lifetime offsets and admit model
    // ranges without allowing retirement to overwrite in-flight geometry.
    {
        neptune::GeometryArena<4096> physical(16u*1024u*1024u);
        auto topology=physical.allocate(349488,64);
        auto geometry=physical.allocate(1310580,64);
        auto shade=physical.allocate(786348,64);
        assert(topology && geometry && shade);
        assert(topology.offset==0 && geometry.offset==349504 && shade.offset==1660096);
        const auto permanent=physical.used();
        auto model=physical.allocate(256u*1024u,64);
        assert(model && model.offset>=shade.offset+shade.bytes);
        assert(physical.markUsed(model,7) && physical.retire(model));
        assert(physical.completeThrough(6));
        assert(physical.used()==permanent+model.bytes);
        auto another=physical.allocate(model.bytes,64);
        assert(another && another.offset!=model.offset);
        assert(physical.retire(another));
        assert(physical.completeThrough(7) && physical.used()==permanent);
        auto reused=physical.allocate(model.bytes,64);
        assert(reused && reused.offset==model.offset && reused.id!=model.id);
        assert(physical.retire(reused));
        assert(physical.count()==3 && physical.used()==permanent);
    }
    std::puts("PASS Neptune geometry arena range and retirement");
}
