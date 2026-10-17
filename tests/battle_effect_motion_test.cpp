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
    assert((value.acceleration==std::array<std::int32_t,3>{0,0,0}));
    assert((value.velocity==std::array<std::int32_t,3>{-0xAAAAAA,0xAAAAAA,0xAAAAAA}));
    assert((value.angles==std::array<std::int32_t,3>{0x1000000,0x2000000,-0x1000000}));
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
    assert(draws==10&&maximum.angles[1]==-67107840);
    ParticleMotion particle;
    particle.position={65536,-65536,0};
    advanceParticleMotion(particle,{0,0,0},0);
    assert((particle.position==std::array<std::int32_t,3>{65536,-65536,0}));
    advanceParticleMotion(particle,{0,0,0},2);
    assert((particle.position==std::array<std::int32_t,3>{65536,-65536,0}));
    advanceParticleMotion(particle,{0,0,0},1);
    assert((particle.position==std::array<std::int32_t,3>{66159,-66158,0}));
    assert((particle.velocity==std::array<std::int32_t,3>{623,-622,0}));
    assert((particle.acceleration==std::array<std::int32_t,3>{0,0,0}));
    advanceParticleMotion(particle,{0,0,0},1);
    assert(particle.position[0]==67379&&particle.velocity[0]==1220);
    assert(add(INT32_MAX,1)==INT32_MIN&&subtract(INT32_MIN,1)==INT32_MAX);
    assert(angleDifference(0x0FFFFFFF)==-1);
    assert(angleDifference(0x08000000)==-134217728);
    assert(angleDifference(0x07FFFFFF)==134217727);
    std::array<Motion,2> ring{};
    ring[1].angles[0]=65536;
    draws=0;
    auto noImpulse=[&](){++draws;return 1u;};
    auto noTrig=[](unsigned)->int{assert(false);return 0;};
    advanceMotionRing(ring,noImpulse,noTrig,noTrig);
    assert(draws==2&&ring[0].angles[0]==1291&&ring[1].angles[0]==64270);
    assert(ring[0].acceleration[0]==0&&ring[1].acceleration[0]==0);
    std::array<Motion,1> single{};
    std::array<std::uint32_t,7> impulseRandom={0,0x80000000u,0x40000000u,0x40000000u,0,0x40000000u,0x80000000u};
    draws=0;
    advanceMotionRing(single,[&](){assert(draws<7);return impulseRandom[draws++];},sine,cosine);
    assert(draws==7);
    std::array<std::uint32_t,7> seedRandom={0,0,0,0x40000000u,0,0x80000000u,0x80000000u};
    draws=0;
    const auto seed=initializeParticleSeed(4,0x1800,[&](){assert(draws<7);return seedRandom[draws++];},sine,cosine);
    assert(draws==7&&seed.type==0&&seed.delay==0&&seed.field2C==-40&&seed.state==0);
    assert((seed.motion.position==std::array<std::int32_t,3>{-0x1800,0x1800,0}));
    assert((seed.motion.velocity==std::array<std::int32_t,3>{0,0,0}));
    draws=0;
    const auto upperSeed=initializeParticleSeed(4,0x1800,[&](){++draws;return 0xFFFFFFFFu;},
        [](unsigned a){assert(a==4095);return 0;},[](unsigned a){assert(a==4095);return 65536;});
    assert(draws==7&&upperSeed.type==2&&upperSeed.delay==49&&upperSeed.field2C==-203);
    assert((single[0].velocity==std::array<std::int32_t,3>{-18361,36723,18362}));
    assert(single[0].angles[0]==0x10000000-18361);
    advanceMotionRing(std::span<Motion>{},noImpulse,noTrig,noTrig);
    assert(draws==7);
    puts("PASS: native effect motion layout, RNG order, trig indices and signed fixed-point initialization. Producer integration remains pending.");
}
