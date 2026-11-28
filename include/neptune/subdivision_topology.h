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

// Convert a run from flattened polygon slots to one model's local slots.
// The same immutable index topology is used; both vertex streams are
// rebased to the start of that model. Reject runs crossing model boundaries.
inline bool localSubdivisionIndexOffset(std::size_t modelFirst,
    std::size_t modelQuads, std::size_t firstIndex, std::size_t indexCount,
    std::size_t& localIndex) noexcept
{
    if(!modelQuads || modelFirst>kSubdivisionMaxQuads ||
        modelQuads>kSubdivisionMaxQuads-modelFirst || !indexCount ||
        firstIndex%kSubdivisionIndicesPerQuad ||
        indexCount%kSubdivisionIndicesPerQuad) return false;
    const auto first=firstIndex/kSubdivisionIndicesPerQuad;
    const auto count=indexCount/kSubdivisionIndicesPerQuad;
    if(first<modelFirst || first-modelFirst>=modelQuads ||
        count>modelQuads-(first-modelFirst)) return false;
    localIndex=(first-modelFirst)*kSubdivisionIndicesPerQuad;
    return true;
}

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
