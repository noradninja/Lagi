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
inline void prepare_point_corner_matrix(const float world[3],
    const std::int32_t* view, std::int32_t matrix[12])
{
    const std::int32_t identity[12]{0x10000,0,0,0, 0,0x10000,0,0,
        0,0,0x10000,0};
    std::copy(identity, identity+12, matrix);
    if (!view) {
        // Identity view is the common transient-light path. The old loop's
        // zero terms leave exactly this one product per row.
        for (unsigned row=0;row<3;++row)
            matrix[row*4+3]=static_cast<std::int32_t>(std::clamp(
                std::round(65536.0*double(world[row])),
                double(std::numeric_limits<std::int32_t>::min()),
                double(std::numeric_limits<std::int32_t>::max())));
        return;
    }
    for (unsigned row=0;row<3;++row) {
        double v = double(view[row*4+3]);
        for (unsigned col=0;col<3;++col)
            v += double(view[row*4+col])*double(world[col]);
        matrix[row*4+3]=static_cast<std::int32_t>(std::clamp(std::round(v),
            double(std::numeric_limits<std::int32_t>::min()),
            double(std::numeric_limits<std::int32_t>::max())));
    }
}

inline bool evaluate_point_at_corner_prepared(const std::int32_t matrix[12],
    const std::int32_t params[5], const std::uint16_t color[3],
    const std::int32_t basis[9], const std::int32_t* view,
    AdditionalModelLight& out)
{

    // Broad 64-bit rejection prevents distant corners from entering the
    // native square-root/division loop, and avoids the native 32-bit wrap
    // classifying an extremely remote point as nearby.
    const std::uint64_t maxSquared=std::uint64_t(std::uint32_t(params[3]))<<16;
    std::uint64_t distanceSquared=0;
    std::int32_t delta[3]{};
    for(unsigned axis=0;axis<3;++axis) {
        const std::int64_t difference=
            std::int64_t(params[axis])-matrix[axis*4+3];
        if(difference < -6000000 || difference > 6000000) return false;
        delta[axis]=static_cast<std::int32_t>(difference);
        distanceSquared+=std::uint64_t(difference*difference);
        if(distanceSquared>maxSquared) return false;
    }
    // The broad-phase differences are now proven representable as signed
    // 32-bit words, so reuse them in the unchanged SH-2 sqrt/divide path.
    const auto result=native_point_light_from_delta(
        matrix,params,delta,distanceSquared);
    if(result.outside || result.divisionOverflow) return false;

    out={};
    out.active=true;
    for(unsigned axis=0;axis<3;++axis) {
        std::int64_t rotated=0;
        for(unsigned row=0;row<3;++row) {
            if (!view) {
                rotated+=std::int64_t(basis[row*3+axis])*result.vector[row];
            } else {
                std::int64_t modelColumn=0;
                for(unsigned col=0;col<3;++col)
                    modelColumn+=std::int64_t(view[row*4+col]) *
                        basis[col*3+axis];
                rotated+=(modelColumn>>16)*result.vector[row];
            }
        }
        out.vector[axis]=static_cast<std::int16_t>(rotated>>16);
        out.color[axis]=color[axis];
    }
    return true;
}

inline bool evaluate_point_at_corner(const float world[3],
    const std::int32_t params[5], const std::uint16_t color[3],
    const std::int32_t basis[9], const std::int32_t* view,
    AdditionalModelLight& out)
{
    std::int32_t matrix[12];
    prepare_point_corner_matrix(world, view, matrix);
    return evaluate_point_at_corner_prepared(matrix, params, color,
        basis, view, out);
}
}
