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
    std::size_t local=~std::size_t(0);
    assert(neptune::localSubdivisionIndexOffset(123,4,124*24,2*24,local));
    assert(local==24);
    for(unsigned k=0;k<48;++k)
        assert(indices[124*24+k]==123*9+indices[local+k]);
    assert(!neptune::localSubdivisionIndexOffset(123,4,122*24,2*24,local));
    assert(!neptune::localSubdivisionIndexOffset(123,4,126*24,2*24,local));
    assert(!neptune::localSubdivisionIndexOffset(123,4,124*24+1,24,local));
    assert(!neptune::localSubdivisionIndexOffset(123,4,124*24,0,local));
    assert(!neptune::localSubdivisionIndexOffset(quads,1,quads*24,24,local));
    assert(neptune::localSubdivisionIndexOffset(quads-1,1,(quads-1)*24,24,local));
    assert(local==0);
}
