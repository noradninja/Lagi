#include "neptune/subdivision_streams.h"
#include <cassert>
#include <cstring>
#include <type_traits>

int main()
{
    using neptune::SubdivisionGeometryVertex;
    using neptune::SubdivisionShadeVertex;
    static_assert(std::is_standard_layout<SubdivisionGeometryVertex>::value,"geometry layout");
    static_assert(std::is_standard_layout<SubdivisionShadeVertex>::value,"shade layout");
    static_assert(std::is_trivially_copyable<SubdivisionGeometryVertex>::value,"geometry copy");
    static_assert(std::is_trivially_copyable<SubdivisionShadeVertex>::value,"shade copy");
    const float original[8]={1.f,-2.f,3.f,0.125f,0.875f,-0.5f,0.f,0.25f};
    SubdivisionGeometryVertex geometry{original[0],original[1],original[2],original[3],original[4]};
    SubdivisionShadeVertex shade{original[5],original[6],original[7]};
    assert(std::memcmp(&geometry,original,sizeof geometry)==0);
    assert(std::memcmp(&shade,original+5,sizeof shade)==0);
    const auto initial=neptune::interleaveSubdivisionVertex(geometry,shade);
    assert(std::memcmp(&initial,original,sizeof initial)==0);
    shade={0.75f,-0.125f,0.5f};
    assert(std::memcmp(&geometry,original,sizeof geometry)==0);
    const auto repacked=neptune::interleaveSubdivisionVertex(geometry,shade);
    assert(std::memcmp(&repacked,&geometry,sizeof geometry)==0);
    assert(std::memcmp(&repacked.shadeR,&shade,sizeof shade)==0);
    geometry.x=-8.f; geometry.u=0.375f;
    const auto moved=neptune::interleaveSubdivisionVertex(geometry,shade);
    assert(moved.x==-8.f && moved.u==0.375f);
    assert(std::memcmp(&moved.shadeR,&shade,sizeof shade)==0);
}
