#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
using u8=std::uint8_t;using u32=std::uint32_t;
using s32=std::int32_t;using s64=std::int64_t;
struct Stats {s32 m54,m58_itemsUsed,m5C_gunShotFired,m60_homingLaserFired,m64;};
static struct {Stats gameStats;} mainGameState;
#include "stat_count_body.inc"
int main() {
    const int selectors[]={8,0,1,2,3};
    const s32 values[]={0,1,-1,0x7fffffff,std::numeric_limits<s32>::min()};
    for(unsigned selected=0;selected<5;++selected) for(auto value:values)
    for(int high=-128;high<128;++high) {
        mainGameState.gameStats={11,22,33,44,55};
        s32* counters[]={&mainGameState.gameStats.m54,&mainGameState.gameStats.m58_itemsUsed,
            &mainGameState.gameStats.m5C_gunShotFired,&mainGameState.gameStats.m60_homingLaserFired,
            &mainGameState.gameStats.m64};
        *counters[selected]=value;
        s32 before[5];for(unsigned i=0;i<5;++i)before[i]=*counters[i];
        increaseStatsCount(high*256+selectors[selected]);
        for(unsigned i=0;i<5;++i)
            assert(u32(*counters[i])==u32(before[i])+(i==selected?1u:0u));
    }
    std::puts("PASS: actual generated shared statistic dispatcher, native selector-8 storage, byte argument semantics and 6400 wrap/isolation cases; not hardware");
}
