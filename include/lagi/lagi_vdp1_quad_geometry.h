#pragma once
#include <array>
#include <cmath>

namespace lagi {
using NativeScreenQuad = std::array<std::array<float, 2>, 4>;
// VDP1 command-2 coordinates name inclusive pixels, not triangle boundaries.
// Sega's VDP1 manual 7.6 explicitly retains a pixel when all corners coincide.
// Preserve coverage for the two paired-edge collapses used by projected rays;
// leave noncollapsed triangles/quads and arbitrary twisted geometry untouched.
inline bool expand_native_collapsed_quad(NativeScreenQuad& q)
{
    const auto equal = [&](unsigned a, unsigned b) {return q[a] == q[b];};
    if (equal(0,1) && equal(0,2) && equal(0,3)) {
        const auto p=q[0];
        q={{{p[0]-0.5f,p[1]-0.5f},{p[0]+0.5f,p[1]-0.5f},
            {p[0]+0.5f,p[1]+0.5f},{p[0]-0.5f,p[1]+0.5f}}};
        return true;
    }
    const bool across = equal(0,3) && equal(1,2);
    const bool down = equal(0,1) && equal(2,3);
    if (!across && !down) return false;
    const auto a=q[0], b=q[across ? 1 : 2];
    float dx=b[0]-a[0],dy=b[1]-a[1];
    const float length=std::sqrt(dx*dx+dy*dy);
    if (!(length>0)) return false;
    dx/=length;dy/=length;
    const float nx=-dy*0.5f,ny=dx*0.5f;
    const std::array<float,2> start={a[0]-dx*0.5f,a[1]-dy*0.5f};
    const std::array<float,2> end={b[0]+dx*0.5f,b[1]+dy*0.5f};
    if (across)
        q={{{start[0]+nx,start[1]+ny},{end[0]+nx,end[1]+ny},
            {end[0]-nx,end[1]-ny},{start[0]-nx,start[1]-ny}}};
    else
        q={{{start[0]+nx,start[1]+ny},{start[0]-nx,start[1]-ny},
            {end[0]-nx,end[1]-ny},{end[0]+nx,end[1]+ny}}};
    return true;
}
}
