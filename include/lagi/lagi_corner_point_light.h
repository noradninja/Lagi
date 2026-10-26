#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include "lagi/lagi_additive_light.h"
#include "lagi/lagi_point_light.h"

namespace lagi {
// Shared evaluation of a native Saturn point source against one authored
// Gouraud corner. All sources contribute to the SAME RGB accumulator and
// are quantized only once after all lights have been accumulated.
//
// params are the original SH-2 native evaluator's five fixed-point words.
// When view is supplied both the vertex and source are already expected in
// the same view-space frame as params. A null view means world-space params.
// basis is the object's world-space 3x3 rotation (16.16), rows first.
inline bool evaluate_point_at_corner(const float world[3],
    const std::int32_t params[5], const std::uint16_t color[3],
    const std::int32_t basis[9], const std::int32_t* view,
    AdditionalModelLight& out)
{
    std::int32_t matrix[12]{0x10000,0,0,0, 0,0x10000,0,0, 0,0,0x10000,0};
    for (unsigned row=0;row<3;++row) {
        double v = view ? double(view[row*4+3]) : 0.0;
        for (unsigned col=0;col<3;++col)
            v += (view ? double(view[row*4+col]) : (row==col ? 65536.0:0.0))
                * double(world[col]);
        matrix[row*4+3]=static_cast<std::int32_t>(std::clamp(std::round(v),
            double(std::numeric_limits<std::int32_t>::min()),
            double(std::numeric_limits<std::int32_t>::max())));
    }

    // Broad 64-bit rejection prevents distant corners from entering the
    // native square-root/division loop, and avoids the native 32-bit wrap
    // classifying an extremely remote point as nearby.
    const std::uint64_t maxSquared=std::uint64_t(std::uint32_t(params[3]))<<16;
    std::uint64_t distanceSquared=0;
    for(unsigned axis=0;axis<3;++axis) {
        const std::int64_t delta=std::int64_t(params[axis])-matrix[axis*4+3];
        if(delta < -6000000 || delta > 6000000) return false;
        distanceSquared+=std::uint64_t(delta*delta);
        if(distanceSquared>maxSquared) return false;
    }
    const auto result=native_point_light(matrix,params);
    if(result.outside || result.divisionOverflow) return false;

    out={};
    out.active=true;
    for(unsigned axis=0;axis<3;++axis) {
        std::int64_t rotated=0;
        for(unsigned row=0;row<3;++row) {
            std::int64_t modelColumn=0;
            for(unsigned col=0;col<3;++col)
                modelColumn+=
                    (view ? std::int64_t(view[row*4+col]) :
                        (row==col ? 65536LL : 0LL)) *
                    basis[col*3+axis];
            rotated+=(modelColumn>>16)*result.vector[row];
        }
        out.vector[axis]=static_cast<std::int16_t>(rotated>>16);
        out.color[axis]=color[axis];
    }
    return true;
}
}
