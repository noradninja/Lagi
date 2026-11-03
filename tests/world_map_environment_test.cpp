#include "lagi/world_map_environment.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>

int main()
{
    using lagi::worldmap::map_normal_background_origin;
    // Separate mathematical oracle for the arithmetic shift and native
    // truncation. Do not use the implementation's shift/mask expression.
    const std::int32_t values[] = {
        INT32_MIN, INT32_MIN+1, -0x10001, -4097, -4096, -1,
        0, 1, 195, 196, 197, 235, 236, 237, 4095, 4096,
        0x7FFFFFFE, INT32_MAX,
    };
    unsigned cases=0;
    for(const auto yaw : values)
        for(const auto ground : values)
            for(const auto offset : values) {
                const auto actual=map_normal_background_origin(yaw,ground,offset);
                auto shifted=std::int64_t(yaw)/4096;
                if(yaw<0 && yaw%4096) --shifted;
                const auto x=std::uint32_t(-shifted*1024);
                const auto vertical=std::uint32_t((256-std::int64_t(ground))*65536);
                const auto sum=std::uint32_t(std::int64_t(ground)+offset);
                const auto signedSum=sum<=INT32_MAX ? std::int64_t(sum)
                                                   : std::int64_t(sum)-0x100000000LL;
                const auto y=signedSum>=196 ? std::uint32_t(-signedSum*65536)
                                           : 0xFF140000u;
                assert(actual.horizontal==x && actual.vertical==vertical && actual.scrollY==y);
                ++cases;
            }
    assert(map_normal_background_origin(0,195,0).scrollY==0xFF140000u);
    assert(map_normal_background_origin(0,196,0).scrollY==0xFF3C0000u);
    assert(map_normal_background_origin(0,236,0).scrollY==0xFF140000u);
    std::printf("PASS %u WORLD background-origin cases, signed threshold and wrap; live hookup pending\n",cases);
}
