#pragma once
namespace lagi {
// Translation only: retain native aspect, size and animation offsets.
constexpr float uiLeftEdgeShift(float nativeLeft,float aspectCorrection) {
    return -1.f-nativeLeft/176.f*aspectCorrection;
}
}
