#pragma once
#include <cstdint>
#include <array>
#include <span>

namespace lagi::worldmap {
inline std::int32_t map_environment_multiply(std::int32_t a,std::int32_t b)
{
    return static_cast<std::int32_t>((std::int64_t(a)*b)>>16);
}

// WORLD 06056D7C: MAIN's fixed-point divider remains the arithmetic authority.
// Return its high half sign-extended as native SHLR16/EXTS.W does.
template<class Divide>
inline std::int16_t map_projected_ground(
    const std::array<std::int32_t,3>& row,
    const std::array<std::int16_t,3>& pivot,std::int32_t zStart,Divide divide)
{
    const auto z=static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(std::int32_t(pivot[2]))<<16)-
        static_cast<std::uint32_t>(zStart));
    const auto sum=std::uint32_t(std::int64_t(row[0])*pivot[0])+
        std::uint32_t(std::int64_t(row[1])*pivot[1])+
        std::uint32_t(map_environment_multiply(row[2],z));
    const auto result=divide(static_cast<std::int32_t>(sum),row[1]);
    return static_cast<std::int16_t>(static_cast<std::uint32_t>(result)>>16);
}

struct MapCoefficientWave {
    std::int32_t speed=0;      // environment +48
    std::int32_t frequency=0;  // +4C
    std::int32_t amplitude=0;  // +50
    std::uint32_t phase=0;     // +54
};

// WORLD 0605676E / 060568BE..F4. These are initial task values, not
// renderer-selected camera settings. Desktop keyboard edits are not input.
struct MapEnvironment {
    std::int32_t planeHeight=0x1FB33; // +3C
    std::int32_t verticalOffset=0x13B; // +40
    std::int32_t scale=0x5B333;        // +44
    MapCoefficientWave wave{0x11102A,0x5B059,0x2B3,0};
    std::uint32_t driftX=0;           // +5C; +58 speed = -409
    std::uint32_t driftZ=0;           // +64; +60 speed = -81
};

// WORLD 06056AD4..06056B00, before palette/debug keyboard services.
inline void advance_map_environment(MapEnvironment& environment)
{
    environment.driftX-=409u;
    environment.driftZ-=81u;
}

// WORLD 06057024..6C. Original work-area camera is a per-draw copy; do
// not feed these texture-coordinate adjustments back into the live camera.
template<class Divide>
inline std::array<std::int32_t,3> map_environment_camera(
    std::array<std::int32_t,3> camera,const MapEnvironment& environment,Divide divide)
{
    const auto reciprocal=divide(0x10000,environment.scale);
    const auto xOffset=map_environment_multiply(0x200000,reciprocal);
    // Both PC-relative loads (0605703E and 06057044) resolve to 060571C0.
    const auto zOffset=xOffset;
    camera[0]=static_cast<std::int32_t>(std::uint32_t(camera[0])-
        environment.driftX+std::uint32_t(xOffset));
    camera[2]=static_cast<std::int32_t>(std::uint32_t(camera[2])-
        environment.driftZ+std::uint32_t(zOffset));
    return camera;
}

// WORLD 060565C0. Caller supplies the selected live rotation coefficient
// table and native sine lookup. Reject malformed storage before mutation;
// an undersized table must not silently run a shortened native loop.
template<class Coefficient,class Sine>
inline bool apply_map_coefficient_wave(std::span<Coefficient> table,
                                      MapCoefficientWave& wave,Sine sine)
{
    if(table.size()<0x1A8) return false;
    auto phase=wave.phase;
    for(unsigned i=0;i<0x1A8;++i) {
        const auto modulation=std::uint32_t(map_environment_multiply(
            wave.amplitude,sine((phase>>16)&0xFFF)))+0x10000u;
        table[i]=map_environment_multiply(table[i].asS32(),
                                         static_cast<std::int32_t>(modulation));
        phase+=static_cast<std::uint32_t>(wave.frequency);
    }
    wave.phase+=static_cast<std::uint32_t>(wave.speed);
    return true;
}

struct MapNormalBackgroundOrigin {
    std::uint32_t horizontal;
    std::uint32_t vertical;      // environment +8, before the consumer branch
    std::uint32_t scrollY;       // value passed to MAIN 06028986
};

// WORLD 06056E58..84 and 06056E94..06056EF8. The consumer compares the
// signed, wrapping sum against 196; it is not a conventional clamp to 236.
// Inputs remain native 16.16 yaw, signed projected ground, and environment +40.
inline MapNormalBackgroundOrigin map_normal_background_origin(
    std::int32_t yaw, std::int32_t ground, std::int32_t verticalOffset)
{
    // Twelve SHAR instructions, then SHLL8/SHLL2/NEG. Spell out unsigned
    // shifts/subtractions so the original low-32-bit wrap is well-defined.
    const auto shiftedYaw = (static_cast<std::uint32_t>(yaw) >> 12) |
        (yaw < 0 ? 0xFFF00000u : 0u);
    const auto horizon = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(ground) +
        static_cast<std::uint32_t>(verticalOffset));
    return {
        0u - (shiftedYaw << 10),
        (256u - static_cast<std::uint32_t>(ground)) << 16,
        horizon >= 196
            ? 0u - (static_cast<std::uint32_t>(horizon) << 16)
            : 0xFF140000u,
    };
}
}
