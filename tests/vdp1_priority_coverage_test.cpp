#include "lagi/vdp1_priority_coverage.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
int main() {
    using namespace lagi;
    assert(vdp1PrioritySelectorBit(0)==0);
    assert(vdp1PrioritySelectorBit(127)==0);
    assert(vdp1PrioritySelectorBit(128)==1);
    assert(vdp1PrioritySelectorBit(129)==2);
    assert(vdp1PrioritySelectorBit(136)==2);
    assert(vdp1PrioritySelectorBit(137)==4);
    assert(vdp1PrioritySelectorBit(145)==8);
    assert(vdp1PrioritySelectorBit(153)==16);
    assert(vdp1PrioritySelectorBit(255)==1);
    const std::uint32_t material[]={0x00000000u,0x81000000u,0x89000000u,0xff000000u};
    const unsigned mask=vdp1PrioritySelectorCoverage(material,4);
    assert(mask==(1u|2u|4u));
    Vdp2SpriteColorState s{};s.priority[0]=0x0204;
    auto coverage=vdp1PriorityCoverage(mask,s,3);
    assert(coverage.below && coverage.above);
    coverage=vdp1PriorityCoverage(2u,s,3);
    assert(!coverage.below && coverage.above);
    coverage=vdp1PriorityCoverage(4u,s,3);
    assert(coverage.below && !coverage.above);
    coverage=vdp1PriorityCoverage(4u,s,2);
    assert(!coverage.below && coverage.above); // VDP1 wins a tie.
    std::puts("PASS native selector coverage and register-derived priority bands");
}
