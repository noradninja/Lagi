#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace lagi::camp::recovery {
// Logical host state for CAMP+614..620. Never overlay native task offsets
// onto the polymorphic C++ work area. Both color arrays use signed MOV.B.
struct ColorTransition {
    std::array<std::int8_t,3> source{},target{};
    std::int32_t tick=0,duration=0;
};
inline void begin_color_transition(ColorTransition& state,std::uint32_t source,
    std::uint32_t target,std::int32_t duration)
{
    // CAMP060555E2: low byte is red, then green and blue. No RGB555 decode.
    for(unsigned i=0;i<3;++i) {
        state.source[i]=std::bit_cast<std::int8_t>(std::uint8_t(source>>(i*8)));
        state.target[i]=std::bit_cast<std::int8_t>(std::uint8_t(target>>(i*8)));
    }
    state.tick=0;state.duration=duration;
}
// CAMP06055634..0605570E. Pass Azel's FP_Div/MTH_Mul as services. In
// particular duration0 is NOT C++ division by zero: the native divider's
// result is retained, and equal source/target channels still produce source.
template<class Divide,class Multiply>
std::uint32_t step_color_transition(ColorTransition& state,Divide divide,Multiply multiply)
{
    if(state.tick<state.duration)++state.tick;
    const auto factor=divide(state.tick,state.duration);
    std::uint32_t packed=0;
    for(unsigned i=0;i<3;++i) {
        const auto delta=std::int32_t(state.target[i])-std::int32_t(state.source[i]);
        const auto value=std::uint32_t(std::int32_t(state.source[i])+multiply(delta,factor));
        packed|=(value&255u)<<(i*8);
    }
    return packed;
}
struct RecoveryLight {
    std::uint32_t highlight=0x001F1F1F;
    std::int32_t highlightDelay=30,restoreDelay=120;
    ColorTransition transition;
};
inline void begin_recovery_light(RecoveryLight& state,std::uint32_t original)
{
    state.highlight=0x001F1F1F;state.highlightDelay=30;state.restoreDelay=120;
    begin_color_transition(state.transition,original,original,0);
}
template<class Divide,class Multiply>
std::uint32_t step_recovery_light(RecoveryLight& state,std::uint32_t original,
    Divide divide,Multiply multiply)
{
    // Order matters when both delays expire together: restoration wins.
    if(state.highlightDelay!=0 && --state.highlightDelay==0)
        begin_color_transition(state.transition,original,state.highlight,30);
    if(state.restoreDelay!=0 && --state.restoreDelay==0)
        begin_color_transition(state.transition,state.highlight,original,30);
    return step_color_transition(state.transition,divide,multiply);
}
// CAMP060557E8..0605583C /06055906..0605595E. +5E4 is the dragon's
// saved position despite the translated field's camera-position name.
template<class Multiply>
std::array<std::int32_t,3> interpolate_target(
    const std::array<std::int32_t,3>& dragon,
    const std::array<std::int32_t,3>& cameraTarget,std::int32_t factor,Multiply multiply)
{
    std::array<std::int32_t,3> result;
    for(unsigned i=0;i<3;++i) {
        const auto delta=std::bit_cast<std::int32_t>(std::uint32_t(cameraTarget[i])-std::uint32_t(dragon[i]));
        result[i]=std::bit_cast<std::int32_t>(std::uint32_t(dragon[i])+std::uint32_t(multiply(delta,factor)));
    }
    return result;
}
}
