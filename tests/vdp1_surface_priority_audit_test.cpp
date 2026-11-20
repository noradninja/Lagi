#include "lagi/vdp1_surface_priority_audit.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
int main() {
    lagi::Vdp2SpriteColorState s{};
    s.prir=3;s.priority[0]=0x0204;
    // A8B8G8R8 high byte; code 129 => bank0 priority4, code137
    // => bank1 priority2, directRGB => bank0 priority4.
    const std::uint32_t px[4]={0u,0x81000000u,0x89000000u,0xff000000u};
    const auto a=lagi::auditVdp1SurfacePriorities(px,4,1,4,1,s);
    assert(a.samples==4 && a.empty==1 && a.indexed==2 && a.direct==1);
    assert(a.below==1 && a.above==2 && a.equal==0);
    const auto invalid=lagi::auditVdp1SurfacePriorities(px,5,1,4,1,s);
    assert(invalid.samples==0);
    std::puts("PASS completed VDP1 surface priority sampling");
}
