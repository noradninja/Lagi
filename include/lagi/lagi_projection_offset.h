#pragma once

#include <cmath>

namespace lagi {

// Equivalent to Azel's post-projection NDC translation. Pull the view-space
// shear back into world space so models with different local origins can
// retain Neptune's shared GPU batch and unchanged depth/lighting inputs.
inline bool nativeProjectionOffset(int localX, int localY, float canvasHalfWidth,
    float projectionX, float projectionY, const float view[12], float delta[3])
{
    delta[0] = delta[1] = delta[2] = 0.0f;
    if ((localX == 176 && localY == 112) || projectionX == 0 || projectionY == 0)
        return false;
    const float x = (localX - 176) / 176.0f * canvasHalfWidth / projectionX;
    const float y = -(localY - 112) / 112.0f / projectionY;
    const float a=view[0], b=view[1], c=view[2];
    const float d=view[4], e=view[5], f=view[6];
    const float g=view[8], h=view[9], i=view[10];
    const float determinant = a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    if (std::fabs(determinant) < 1.e-8f)
        return false;
    delta[0] = ((e*i-f*h)*x + (c*h-b*i)*y) / determinant;
    delta[1] = ((f*g-d*i)*x + (a*i-c*g)*y) / determinant;
    delta[2] = ((d*h-e*g)*x + (b*g-a*h)*y) / determinant;
    return true;
}

} // namespace lagi
