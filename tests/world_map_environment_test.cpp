#include "lagi/world_map_environment.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

struct Coefficient {
    std::int32_t value=0;
    Coefficient(std::int32_t v=0):value(v){}
    std::int32_t asS32()const{return value;}
};

static std::int32_t multiply_oracle(std::int32_t a,std::int32_t b)
{
    const auto product=std::int64_t(a)*b;
    auto quotient=product/65536;
    if(product<0 && product%65536) --quotient;
    return static_cast<std::int32_t>(quotient);
}

int main()
{
    struct Registers {
        unsigned mEC_CCCTL=0x10,mE0_SPCTL=0x20,m100_CCRSA=0,m102_CCRSB=3,
            m104_CCRSC=4,m106_CCRSD=5,m10C_CCRR=0;
    } registers;
    lagi::worldmap::initialize_map_environment_color(registers);
    assert(registers.mEC_CCCTL==0x150 && registers.mE0_SPCTL==0x1323);
    assert(registers.m100_CCRSA==0x180D && registers.m102_CCRSB==0 &&
        registers.m104_CCRSC==0 && registers.m106_CCRSD==0 && registers.m10C_CCRR==16);
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
                const auto y=signedSum>=236 ? std::uint32_t(-signedSum*65536)
                                           : 0xFF140000u;
                assert(actual.horizontal==x && actual.vertical==vertical && actual.scrollY==y);
                ++cases;
            }
    assert(map_normal_background_origin(0,195,0).scrollY==0xFF140000u);
    assert(map_normal_background_origin(0,196,0).scrollY==0xFF140000u);
    assert(map_normal_background_origin(0,236,0).scrollY==0xFF140000u);
    lagi::worldmap::MapEnvironment environment;
    environment.driftX=0x100u;environment.driftZ=0xFFFFFFFFu;
    lagi::worldmap::advance_map_environment(environment);
    assert(environment.driftX==0xFFFFFF67u && environment.driftZ==0xFFFFFFAEu);
    const auto adjusted=lagi::worldmap::map_environment_camera({100,200,300},environment,
        [](std::int32_t numerator,std::int32_t denominator){
            assert(numerator==0x10000 && denominator==0x5B333);return 0x10000;
        });
    assert(adjusted[0]==100+153+0x200000 && adjusted[1]==200 && adjusted[2]==300+82+0x200000);
    const auto sine=[](unsigned index){return std::int32_t(index*31u)-65536;};
    for(const auto phase : {0u,0xFFFFFFFFu,0x80008000u})
        for(const auto amplitude : {-65536,0,691,INT32_MAX}) {
            lagi::worldmap::MapCoefficientWave wave{0x11102A,0x5B059,amplitude,phase};
            std::vector<Coefficient> table(0x1A9);
            for(unsigned i=0;i<table.size();++i)table[i]=std::int32_t(i*131071u)-0x1000000;
            auto expected=table;
            auto localPhase=phase;
            for(unsigned i=0;i<0x1A8;++i) {
                const auto factor=static_cast<std::int32_t>(0x10000u+
                    std::uint32_t(multiply_oracle(amplitude,sine((localPhase>>16)&4095))));
                expected[i]=multiply_oracle(expected[i].value,factor);
                localPhase+=0x5B059u;
            }
            unsigned calls=0;
            assert(lagi::worldmap::apply_map_coefficient_wave(
                std::span(table),wave,[&](unsigned index){++calls;return sine(index);}));
            assert(calls==424 && wave.phase==phase+0x11102Au);
            for(unsigned i=0;i<table.size();++i)assert(table[i].value==expected[i].value);
            const auto completed=wave.phase;
            assert(!lagi::worldmap::apply_map_coefficient_wave(
                std::span(table.data(),423),wave,[&](unsigned){assert(false);return 0;}));
            assert(wave.phase==completed);
            for(unsigned i=0;i<table.size();++i)assert(table[i].value==expected[i].value);
        }
    for(const auto quotient : {0x7FFFFFFFu,0x80000000u,0xFFFF0000u,0x1234ABCDu}) {
        bool called=false;
        const auto ground=lagi::worldmap::map_projected_ground(
            {0x10000,0x20000,0x30000},{-10,20,30},0x40000,
            [&](std::int32_t numerator,std::int32_t denominator){
                called=true;
                assert(numerator==108*65536 && denominator==0x20000);
                return static_cast<std::int32_t>(quotient);
            });
        assert(called && ground==static_cast<std::int16_t>(quotient>>16));
    }
    std::printf("PASS %u WORLD background-origin cases, signed threshold and wrap; hardware output unverified\n",cases);
    std::puts("PASS WORLD 424-entry coefficient wave, phase/suffix/bounds guards and native divider binding; sine/divider services mocked");
}
