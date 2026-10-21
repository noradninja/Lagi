#include "../include/lagi/rbg_format.h"
#include <cassert>
#include <cstdio>
int main() {
    unsigned matches=0;
    for(unsigned ch=0;ch<65536;++ch)for(unsigned top=0;top<4;++top) {
        const unsigned pn=(top<<14)|0x128;
        const bool generic=((ch>>8)&1)==1 && ((ch>>12)&7)==1 &&
            !(ch&0x200) && ((pn>>15)&1)==1 && ((pn>>14)&1)==0;
        assert(lagi::rbgTile16EightBit(ch,pn)==generic);
        matches+=generic;
    }
    assert(matches>0);
    assert(lagi::rbgTile16EightBit(0x1100,0x8008));
    assert(!lagi::rbgTile16EightBit(0x1300,0x8008));
    assert(!lagi::rbgTile16EightBit(0x0100,0x8008));
    puts("PASS exhaustive native RBG format eligibility; shader variant shares generic addressing and coefficients");
}
