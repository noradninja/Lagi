#pragma once
#include <cstdint>
#include <limits>

namespace lagi {
// An additional model-space light contributes only its positive diffuse term.
// Ambient/depth falloff and material color belong to the base lighting once.
struct AdditionalModelLight {
    std::int16_t vector[3]{};
    std::uint16_t color[3]{}; // native reversed packed-color order
    bool active=false;
};
inline void accumulate_additional_model_light(const AdditionalModelLight& light,
    const std::int16_t normal[3],int accum[3])
{
    if(!light.active) return;
    std::int64_t dot=0;
    for(unsigned axis=0;axis<3;++axis) dot+=std::int64_t(normal[axis])*light.vector[axis];
    if(dot<=0) return;
    const auto intensity=dot>>16;
    for(unsigned channel=0;channel<3;++channel) {
        const auto combined=std::int64_t(accum[channel])+light.color[2-channel]*intensity;
        accum[channel]=static_cast<int>(combined>std::numeric_limits<int>::max()
            ? std::numeric_limits<int>::max() : combined);
    }
}
}
