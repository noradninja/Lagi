#include <cassert>
#include <cstdint>
#include <cstdio>
using u16=std::uint16_t;
u16 loc_601FA9E;
u16 projectionStack[8];
u16* projectionStackTop=projectionStack+8;
#include "point_selector_production.inc"
int main()
{
    for (unsigned round=0;round<1000;++round) {
        initProjectionStack_Master();
        dragonFieldTaskDrawSub1Sub0();
        if (loc_601FA9E!=0xA11D || projectionStackTop!=projectionStack+7) return 1;
        pushProjectionStack_Master();
        if (loc_601FA9E!=0x6103 || projectionStackTop!=projectionStack+6) return 2;
        dragonFieldTaskDrawSub1Sub0();
        dragonFieldTaskDrawSub3Sub1();
        if (loc_601FA9E!=0x6103 || projectionStackTop!=projectionStack+6) return 3;
        dragonFieldTaskDrawSub3Sub1();
        if (loc_601FA9E!=0xA11D || projectionStackTop!=projectionStack+7) return 4;
        dragonFieldTaskDrawSub3Sub1();
        if (loc_601FA9E!=0x6103 || projectionStackTop!=projectionStack+8) return 5;
    }
    for (unsigned level=0;level<8;++level) dragonFieldTaskDrawSub1Sub0();
    for (unsigned level=0;level<8;++level) dragonFieldTaskDrawSub3Sub1();
    initProjectionStack_Master();
    if (loc_601FA9E!=0x6103 || projectionStackTop!=projectionStack+8) return 6;
    puts("PASS: actual generated selector activation/directional nesting/pop/reset, 1000 lifecycles and all eight slots (host, not Vita)");
}
