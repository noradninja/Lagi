#pragma once
#include <cstdint>
#include <algorithm>
namespace lagi {
// MAIN 06034ADC..06034BF0 expands signed five-bit endpoints to native
// signed-nine-bit color offsets. MAIN 06034C20 packs R/G/B back in that order.
inline int native_fade_component(unsigned packed){
    const int c=int(packed&31)-16;return c*(c>0?17:16);
}
inline unsigned native_fade_pack_component(int offset){
    const auto v=std::int64_t(offset);
    const auto shifted=v>=0?v/16:-((-v+15)/16);
    return unsigned(std::clamp<std::int64_t>(shifted+16,0,31));
}
inline unsigned native_fade_pack(int r,int g,int b){
    return 0x8000u|native_fade_pack_component(r)|
        (native_fade_pack_component(g)<<5)|(native_fade_pack_component(b)<<10);
}
}
