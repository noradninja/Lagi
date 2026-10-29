#include "lagi/lagi_point_light.h"
#include <cstdio>
#include <cmath>
#include <initializer_list>
#include <random>
static std::uint32_t restoring_sqrt(std::uint32_t distance)
{
    std::uint32_t remainder=distance,root=0,bit=0x40000000;
    for(unsigned i=0;i<16;++i) {
        const auto trial=root+bit;
        root>>=1;
        if(trial<=remainder){remainder-=trial;root+=bit;}
        bit>>=2;
    }
    return root;
}
int main()
{
    // Retain the original SH-2 loop as an independent parity oracle.
    for(std::uint32_t root=0;root<65536u;++root) {
        const std::uint64_t square=std::uint64_t(root)*root;
        for(const std::uint64_t value:{square?square-1u:0u,square,square+1u}) {
            if(value<=0xffffffffu &&
                lagi::native_point_integer_sqrt(std::uint32_t(value))!=
                    restoring_sqrt(std::uint32_t(value)))return 9;
        }
    }
    std::mt19937 rng(0x0601FD86u);
    for(unsigned i=0;i<200000u;++i) {
        const std::uint32_t value=rng();
        if(lagi::native_point_integer_sqrt(value)!=restoring_sqrt(value))return 10;
    }
    std::int32_t matrix[12]={65536,0,0,0,0,65536,0,0,0,0,65536,0};
    std::int32_t params[5]={0,0,0,0x14000000,0x19000};
    auto zero=lagi::native_point_light(matrix,params);
    if (!zero.divisionOverflow || zero.vector!=std::array<std::int16_t,3>{}) return 1;
    params[0]=1;
    const auto tiny=lagi::native_point_light(matrix,params);
    if (!tiny.divisionOverflow || tiny.vector[0]!=32767) return 7;
    params[0]=-1;
    if (lagi::native_point_light(matrix,params).vector[0]!=-32768) return 8;
    unsigned cases=0;
    for (int distance=1;distance<0x600000;distance+=127) {
        params[0]=distance;
        const auto result=lagi::native_point_light(matrix,params);
        const unsigned middle=unsigned((std::uint64_t(distance)*distance)>>16);
        if (result.distance!=middle) return 2;
        if (middle<unsigned(params[4])) {
            const unsigned root=unsigned(std::sqrt(double(middle)));
            if (result.divisor!=root || result.outside) return 3;
        } else if (result.outside!=(middle>unsigned(params[3]))) return 4;
        ++cases;
    }
    params[0]=65536;
    const auto positive=lagi::native_point_light(matrix,params);
    params[0]=-65536;
    const auto negative=lagi::native_point_light(matrix,params);
    if (positive.vector[0]!=4096 || negative.vector[0]!=-4096) return 5;
    matrix[3]=-65536;
    if (lagi::native_point_light(matrix,params).vector!=zero.vector) return 6;
    std::printf("PASS: %u native point-light distance/sqrt branches, signed direction, translation and zero-distance overflow (host math; not renderer integration)\n",cases);
}
