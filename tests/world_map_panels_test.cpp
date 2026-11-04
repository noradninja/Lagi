#include "lagi/world_map_panels.h"
#include <array>
#include <cassert>
#include <cstdio>
int main() {
    // Enumerate all combinations of relevant native story bits. Independent
    // oracle applies priority from last to first instead of early-return flow.
    constexpr unsigned bytes[]={9,9,12,8,7,5,15,11,5,4};
    constexpr unsigned masks[]={0x10,0x40,0x40,0x40,0x10,1,1,4,0x40,8};
    for(unsigned bits=0;bits<1024;++bits) {
        std::array<std::uint8_t,16> story{};
        for(unsigned i=0;i<10;++i) if(bits&(1u<<i)) story[bytes[i]]|=masks[i];
        int a=-1,b=-1;
        if(bits&32) a=0;
        if(bits&16) a=1;
        if(bits&8) a=2;
        if(bits&4) a=3;
        if(bits&2) a=4;
        if(bits&1) a=-1;
        if(bits&512) b=0;
        if(bits&256) b=1;
        if(bits&128) b=2;
        if(bits&64) b=3;
        if(bits&4) b=-1;
        assert(lagi::worldmap::map_panel_variant(story,0)==a);
        assert(lagi::worldmap::map_panel_variant(story,1)==b);
    }
    assert(lagi::worldmap::map_panel_variant({},0)==-1);
    std::array<std::uint8_t,16> story{};
    assert(lagi::worldmap::map_panel_variant(story,2)==-1);
    puts("PASS 2048 WORLD story-panel selection cases; sprite producer not yet connected");
}
