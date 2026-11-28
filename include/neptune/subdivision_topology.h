#pragma once
#include <cstddef>
#include <cstdint>

namespace neptune {
inline constexpr unsigned kSubdivisionVerticesPerQuad=9;
inline constexpr unsigned kSubdivisionIndicesPerQuad=24;
// Match the legacy 3x3 grid winding exactly. Absolute vertex slots must
// fit U16. A complete quad must fit, including its final vertex.
inline constexpr unsigned kSubdivisionMaxQuads=65536u/kSubdivisionVerticesPerQuad;
inline constexpr unsigned kSubdivisionGridTriangles[kSubdivisionIndicesPerQuad]={
    0,1,4, 0,4,3,
    1,2,5, 1,5,4,
    3,4,7, 3,7,6,
    4,5,8, 4,8,7
};

inline bool writeSubdivisionTopology(std::uint16_t* destination,
    std::size_t indexCapacity, unsigned quads) noexcept
{
    if (!destination || quads>kSubdivisionMaxQuads ||
        indexCapacity<std::size_t(quads)*kSubdivisionIndicesPerQuad)
        return false;
    for(unsigned p=0;p<quads;++p)
        for(unsigned k=0;k<kSubdivisionIndicesPerQuad;++k)
            destination[p*kSubdivisionIndicesPerQuad+k]=
                static_cast<std::uint16_t>(p*kSubdivisionVerticesPerQuad+
                    kSubdivisionGridTriangles[k]);
    return true;
}
} // namespace neptune
