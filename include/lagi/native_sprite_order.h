#pragma once
#include <algorithm>
namespace lagi {
// PDS flushVdp1: three stable, descending five-bit radix passes. Bit 15
// is not part of the packet key. Keep emission order for equal buckets.
template<class Commands> void orderNativeSprites(Commands& commands) {
    std::stable_sort(commands.begin(), commands.end(), [](const auto& a,const auto& b) {
        const unsigned ka=a.hasNativeSortKey?(a.nativeSortKey&0x7fff):0;
        const unsigned kb=b.hasNativeSortKey?(b.nativeSortKey&0x7fff):0;
        return ka>kb;
    });
}
}
