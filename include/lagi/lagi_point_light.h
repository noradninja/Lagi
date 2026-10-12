#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace lagi {
// Original 0601FD52/0601FCDC. Matrix is the native view/model 3x4,
// not the bridge's view-stripped world matrix. Result is three signed words.
struct NativePointLightResult {
    std::array<std::int16_t,3> vector{};
    std::uint32_t distance = 0;
    std::uint32_t divisor = 0;
    bool outside = false;
    bool divisionOverflow = false;
    bool resolved = true;
};
inline NativePointLightResult native_point_light(
    const std::int32_t matrix[12], const std::int32_t params[5])
{
    NativePointLightResult result;
    std::int32_t delta[3];
    std::uint64_t squared = 0;
    for (unsigned axis=0; axis<3; ++axis) {
        delta[axis]=static_cast<std::int32_t>(std::uint32_t(params[axis])-
            std::uint32_t(matrix[axis*4+3]));
        squared+=static_cast<std::uint64_t>(std::int64_t(delta[axis])*delta[axis]);
    }
    // SH-2 XTRCT retains the middle 32 bits, including wraparound.
    result.distance=static_cast<std::uint32_t>(squared>>16);
    std::uint32_t numerator;
    if (result.distance < std::uint32_t(params[4])) {
        // Native 16-iteration restoring square root, 0601FD86..9E.
        std::uint32_t remainder=result.distance, root=0, bit=0x40000000;
        for (unsigned i=0;i<16;++i) {
            const auto trial=root+bit;
            root>>=1;
            if (trial<=remainder) { remainder-=trial; root+=bit; }
            bit>>=2;
        }
        result.divisor=root;
        numerator=0x00100000;
    } else {
        numerator=std::uint32_t(params[3]);
        if (result.distance>numerator) { result.outside=true; return result; }
        result.divisor=result.distance;
    }
    // DIVU signed 32/32 mode; expose overflow rather than hiding it.
    const auto dividend=static_cast<std::int32_t>(numerator);
    const auto divisor=static_cast<std::int32_t>(result.divisor);
    std::int32_t factor;
    if (!divisor || (dividend==std::numeric_limits<std::int32_t>::min() && divisor==-1)) {
        result.divisionOverflow=true;
        // The all-zero delta produces zero for any hardware quotient. Other
        // overflowing inputs require the exact SH7604 DIVU result before this
        // evaluator can be connected; do not invent saturation behavior.
        result.resolved=delta[0]==0 && delta[1]==0 && delta[2]==0;
        return result;
    } else factor=dividend/divisor;
    for (auto& component:delta)
        component=static_cast<std::int32_t>(static_cast<std::uint64_t>(std::int64_t(component)*factor)>>16);
    for (unsigned axis=0;axis<3;++axis) {
        std::uint64_t mac=0;
        for (unsigned row=0;row<3;++row)
            mac+=static_cast<std::uint64_t>(std::int64_t(matrix[row*4+axis])*delta[row]);
        result.vector[axis]=static_cast<std::int16_t>(mac>>16);
    }
    return result;
}
}
