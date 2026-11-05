#include "lagi/camp_recovery.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <limits>
using namespace lagi::camp::recovery;
static std::int32_t divide(std::int32_t numerator,std::int32_t denominator)
{
    if(!denominator)return numerator<0?INT32_MIN:INT32_MAX;
    return std::int32_t(std::int64_t(numerator)*65536/denominator);
}
static std::int32_t multiply(std::int32_t value,std::int32_t factor)
{return std::int32_t((std::int64_t(value)*factor)>>16);}
int main()
{
    ColorTransition state;
    // Independent oracle uses division/remainder, not signed shifts or byte
    // reinterpretation. Exhaust every signed source/target byte pair.
    unsigned cases=0;
    for(unsigned source=0;source<256;++source)
        for(unsigned target=0;target<256;++target) {
            begin_color_transition(state,source*0x10101u,target*0x10101u,30);
            for(int tick=1;tick<=31;++tick) {
                const auto value=step_color_transition(state,divide,multiply);
                const int s=source<128?int(source):int(source)-256;
                const int t=target<128?int(target):int(target)-256;
                const auto product=std::int64_t(t-s)*(std::min(tick,30)*65536/30);
                auto term=product/65536;
                if(product<0&&product%65536)--term;
                const auto byte=unsigned(s+term)&255u;
                assert(value==byte*0x10101u&&state.tick==std::min(tick,30));++cases;
            }
        }
    assert(cases==2031616);
    RecoveryLight light;
    const std::uint32_t original=0x00100804;
    begin_recovery_light(light,original);
    for(int frame=1;frame<=180;++frame) {
        const auto color=step_recovery_light(light,original,divide,multiply);
        if(frame<30)assert(color==original&&light.transition.duration==0);
        if(frame==30)assert(light.transition.tick==1&&light.transition.duration==30);
        if(frame>=59&&frame<120)assert(color==0x1F1F1F);
        if(frame==120)assert(light.transition.tick==1&&light.transition.target[0]==4);
        if(frame>=149)assert(color==original);
    }
    begin_recovery_light(light,original);light.highlightDelay=light.restoreDelay=1;
    step_recovery_light(light,original,divide,multiply);
    assert(light.transition.source[0]==31&&light.transition.target[0]==4);
    begin_color_transition(state,0xABCDEF,0xABCDEF,0);
    bool called=false;
    const auto zero=step_color_transition(state,[&](int n,int d){assert(!n&&!d);called=true;return INT32_MAX;},multiply);
    assert(called&&zero==0xABCDEF);
    const std::array<std::int32_t,3> dragon={INT32_MIN,20,INT32_MAX};
    const std::array<std::int32_t,3> target={INT32_MAX,-40,INT32_MIN};
    assert(interpolate_target(dragon,target,0,multiply)==dragon);
    assert(interpolate_target(dragon,target,65536,multiply)==target);
    puts("PASS Camp native signed color arithmetic:2031616 cases, delay/ramp/restore ordering, zero-duration divider service and wrapping camera endpoints; not wired or hardware acceptance");
}
