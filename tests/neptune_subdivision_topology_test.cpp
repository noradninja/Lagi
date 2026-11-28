#include "neptune/subdivision_topology.h"
#include <cassert>
#include <vector>

int main()
{
    constexpr unsigned pattern[24]={
        0,1,4,0,4,3,1,2,5,1,5,4,
        3,4,7,3,7,6,4,5,8,4,8,7
    };
    constexpr auto quads=neptune::kSubdivisionMaxQuads;
    std::vector<std::uint16_t> indices(quads*24u+1u,0xffffu);
    assert(!neptune::writeSubdivisionTopology(nullptr,indices.size(),quads));
    assert(!neptune::writeSubdivisionTopology(indices.data(),indices.size(),quads+1));
    assert(!neptune::writeSubdivisionTopology(indices.data(),quads*24u-1u,quads));
    assert(indices.front()==0xffffu);
    assert(neptune::writeSubdivisionTopology(indices.data(),indices.size(),quads));
    for(unsigned p=0;p<quads;++p)
        for(unsigned k=0;k<24;++k)
            assert(indices[p*24u+k]==p*9u+pattern[k]);
    assert(indices.back()==0xffffu);
    assert(quads*9u<=65536u && (quads+1u)*9u>65536u);
}
