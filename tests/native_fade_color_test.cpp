#include "lagi/native_fade_color.h"
#include <cassert>
#include <cstdio>
int main() {
    for(unsigned c=0;c<32768;++c) {
        const auto packed=lagi::native_fade_pack(
            lagi::native_fade_component(c),
            lagi::native_fade_component(c>>5),
            lagi::native_fade_component(c>>10));
        assert(packed==(c|0x8000u));
    }
    for(int x=-256;x<=255;++x) {
        const int floor16=x>=0?x/16:-((-x+15)/16);
        assert(lagi::native_fade_pack_component(x)==unsigned(floor16+16));
    }
    assert(lagi::native_fade_component(0)==-256);
    assert(lagi::native_fade_component(16)==0);
    assert(lagi::native_fade_component(31)==255);
    puts("PASS native fade endpoints, all RGB555 round trips and signed packing");
}
