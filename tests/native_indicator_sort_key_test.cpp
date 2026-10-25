#include "lagi/native_indicator_sort_key.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include "lagi/native_sprite_order.h"
static unsigned native(unsigned r0) {
    r0<<=8;r0<<=2;r0<<=2;r0>>=16;return r0;
}
struct Command {bool hasNativeSortKey=true;unsigned nativeSortKey,id;};
int main() {
    for(unsigned i=0;i<0x100000;++i)assert(lagi::nativeIndicatorSortKey(i)==native(i));
    unsigned seed=0x6a09e667;
    for(unsigned i=0;i<100000;++i){seed=seed*1664525+1013904223;assert(lagi::nativeIndicatorSortKey(seed)==native(seed));}
    assert(lagi::nativeIndicatorSortKey(0x200)==0x20);
    std::vector<Command> commands={{true,0,0},{true,lagi::nativeIndicatorSortKey(0x200),1},{true,0,2}};
    lagi::orderNativeSprites(commands);
    assert(commands[0].id==1 && commands[1].id==0 && commands[2].id==2);
    puts("PASS native indicator SH2 wrap/shift oracle and shared panel ordering; hardware pending");
}
