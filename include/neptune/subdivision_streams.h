#pragma once
#include <cstddef>

namespace neptune {
// Shader inputs are unchanged; only their memory binding differs.
struct SubdivisionGeometryVertex { float x,y,z,u,v; };
struct SubdivisionShadeVertex { float r,g,b; };
static_assert(sizeof(SubdivisionGeometryVertex)==20,"GXM geometry stride");
static_assert(offsetof(SubdivisionGeometryVertex,u)==12,"GXM UV offset");
static_assert(sizeof(SubdivisionShadeVertex)==12,"GXM shade stride");
} // namespace neptune
