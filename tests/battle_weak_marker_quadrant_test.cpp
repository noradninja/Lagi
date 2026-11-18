#include <cassert>
#include <cstdint>
#include "../include/lagi/native_weak_marker_visibility.h"
int main()
{
    using lagi::nativeWeakMarkerVisible;
    for(unsigned q=0;q<4;++q)
        for(unsigned mask=0;mask<16;++mask)
            for(int index=0;index<3;++index)
                for(unsigned flags=0;flags<8;++flags)
                {
                    const auto value=(std::uint32_t(mask)<<28) |
                        ((flags&1)?0x00200000u:0u) |
                        ((flags&2)?0x00100000u:0u) |
                        ((flags&4)?0x00040000u:0u);
                    const bool expected=index>0 && (mask&(8u>>q)) &&
                        (flags&1) && !(flags&2) && !(flags&4);
                    assert(nativeWeakMarkerVisible(value,index,q)==expected);
                }
    // A weak-point selection made while behind must not bleed into front.
    const auto backOnly = 0x20000000u | 0x00200000u;
    assert(!nativeWeakMarkerVisible(backOnly,2,0));
    assert(!nativeWeakMarkerVisible(backOnly,2,1));
    assert(nativeWeakMarkerVisible(backOnly,2,2));
    assert(!nativeWeakMarkerVisible(backOnly,0,2));
    assert(!nativeWeakMarkerVisible(backOnly,2,4));
}
