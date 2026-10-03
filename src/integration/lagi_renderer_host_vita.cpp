#include "lagi/platform.h"

// Azel renderer-facing host hooks. Desktop Azel provides these through its
// BGFX/OpenGL presentation layer; on Vita they terminate at Neptune/GXM.

void RendererSetFov(float fovInDegree)
{
    lagi::platform::renderer::set_fov(fovInDegree);
}

void invalidateCramRange(unsigned int start, unsigned int size)
{
    lagi::platform::renderer::invalidate_cram_range(start, size);
}
