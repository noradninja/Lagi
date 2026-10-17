#include "lagi/battle_effect_motion.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace lagi::battle::effects;
int main()
{
    const std::array<std::uint32_t,10> randomValues={
        0x40000000u,0,0x40000000u,0x80000000u,
        0x80000000u,0x40000000u,0x40000000u,0x80000000u,
        0x40000000u,0x80000000u};
    std::size_t draws=0;
    std::vector<unsigned> sineIndices,cosineIndices;
    auto random=[&](){assert(draws<randomValues.size());return randomValues[draws++];};
    auto sine=[&](unsigned a){sineIndices.push_back(a);assert(a%1024==0);const int v[]={0,65536,0,-65536};return v[a/1024];};
    auto cosine=[&](unsigned a){cosineIndices.push_back(a);assert(a%1024==0);const int v[]={65536,0,-65536,0};return v[a/1024];};
    const auto value=initializeMotion(random,sine,cosine);
    assert(draws==10);
    assert((value.position==std::array<std::int32_t,3>{0,0,0}));
    assert((value.velocity==std::array<std::int32_t,3>{-0xAAAAAA,0xAAAAAA,0xAAAAAA}));
    assert((value.angularVelocity==std::array<std::int32_t,3>{0x1000000,0x2000000,-0x1000000}));
    assert((sineIndices==std::vector<unsigned>{1024,1024,1024,1024}));
    assert((cosineIndices==std::vector<unsigned>{0,2048,2048,2048}));
    assert(multiply(-1,1)==-1); // arithmetic shift, not truncation toward zero
    assert(multiply(65535,65535)==65534);
    // High-bit and low-bit handling: all-one random stays inside native
    // 4096-entry trig tables; magnitude uses only the high sixteen bits.
    draws=0;
    const auto maximum=initializeMotion([&](){++draws;return 0xFFFFFFFFu;},
        [](unsigned a){assert(a==4095);return -65536;},
        [](unsigned a){assert(a==4095);return 65536;});
    assert(draws==10&&maximum.angularVelocity[1]==-67107840);
    puts("PASS: native effect motion layout, RNG order, trig indices and signed fixed-point initialization. Producer integration remains pending.");
}
