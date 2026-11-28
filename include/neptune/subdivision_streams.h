#pragma once
#include <cstddef>

namespace neptune {
// Shader inputs are unchanged; only their memory binding differs.
struct SubdivisionGeometryVertex { float x,y,z,u,v; };
struct SubdivisionShadeVertex { float r,g,b; };
static_assert(sizeof(SubdivisionGeometryVertex)==20,"GXM geometry stride");
static_assert(offsetof(SubdivisionGeometryVertex,u)==12,"GXM UV offset");
static_assert(sizeof(SubdivisionShadeVertex)==12,"GXM shade stride");
struct SubdivisionInterleavedVertex {
    float x,y,z,u,v,shadeR,shadeG,shadeB;
};
static_assert(sizeof(SubdivisionInterleavedVertex)==32,"legacy GXM stride");
static_assert(offsetof(SubdivisionInterleavedVertex,shadeR)==20,"legacy shade offset");

// Repack the current complete vertex, including fields that did not change
// during this frame. A stale legacy buffer is never used as an input.
inline SubdivisionInterleavedVertex interleaveSubdivisionVertex(
    const SubdivisionGeometryVertex& geometry,
    const SubdivisionShadeVertex& shade) noexcept
{
    return {geometry.x,geometry.y,geometry.z,geometry.u,geometry.v,
        shade.r,shade.g,shade.b};
}
} // namespace neptune
