#pragma once
#include <algorithm>
#include <cstdint>

namespace lagi::worldmap {
// WORLD 06055ABC: fixed-point proportional step with signed minimum/maximum
// speed and a final target clamp. Do not replace with floating-point lerp.
inline std::int32_t map_approach(std::int32_t current, std::int32_t target,
    std::int32_t gain, std::int32_t maximum, std::int32_t minimum)
{
    const auto difference = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(target)-static_cast<std::uint32_t>(current));
    auto step = static_cast<std::int32_t>((std::int64_t(difference)*gain)>>16);
    step = difference < 0 ? std::clamp(step,-maximum,-minimum)
                          : std::clamp(step,minimum,maximum);
    const auto remaining = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(difference)-static_cast<std::uint32_t>(step));
    if ((difference < 0 && remaining > 0) ||
        (difference >= 0 && remaining < 0)) return target;
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(current)+
                                     static_cast<std::uint32_t>(step));
}

// WORLD 0605419C's six authored easing profiles.
inline std::int32_t map_approach_profile(std::int32_t current,
    std::int32_t target, unsigned profile)
{
    struct Profile {std::int32_t gain,maximum,minimum;};
    constexpr Profile profiles[] = {
        {0xA3D,0x2000,0xA3}, {0x1999,0x4000,0xF5},
        {0x3333,0x6000,0x199}, {0x1999,0x3E7000,0x28},
        {0x3333,0x3E7000,0x333}, {0x51E,0x1000,0x51}};
    if(profile>=6) return current;
    const auto& p=profiles[profile];
    return map_approach(current,target,p.gain,p.maximum,p.minimum);
}

// WORLD 06054246: ease velocity only while accelerating; apply an exact
// smaller delta immediately when decelerating, so the position reaches target.
inline void map_spring_axis(std::int32_t& position,std::int32_t target,
                            std::int32_t& velocity)
{
    const auto next=map_approach(position,target,0x3333,0x3E7000,0x28);
    const auto delta=static_cast<std::int32_t>(static_cast<std::uint32_t>(next)-
                                            static_cast<std::uint32_t>(position));
    if ((delta>=0 && delta>velocity) || (delta<0 && delta<velocity))
        velocity=map_approach_profile(velocity,delta,3);
    else velocity=delta;
    position=static_cast<std::int32_t>(static_cast<std::uint32_t>(position)+
                                     static_cast<std::uint32_t>(velocity));
}
}
