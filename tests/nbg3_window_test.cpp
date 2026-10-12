#include "lagi/lagi_vdp2_window.h"
#include <cstdio>
#include <initializer_list>
int main()
{
    const unsigned masks[7]={0x8000,0x0200,0x0800,0x2000,0x0100,0x0400,0x1000};
    unsigned cases=0;
    for (unsigned prior=0;prior<65536;++prior)
    for (unsigned command=33;command<=39;++command)
    for (unsigned enabled=0;enabled<2;++enabled) {
        const unsigned mask=masks[command-33];
        const unsigned expected=(prior & ~mask) | (enabled ? mask : 0);
        if (lagi::native_nbg3_window(prior,command,enabled)!=expected) return 1;
        ++cases;
    }
    for (unsigned command : {0u,32u,40u,0xffffffffu})
        if (lagi::native_nbg3_window(0x1234,command,1)!=0x1234) return 2;
    // Actual MENUBK configuration: combine windows, enable both, first inside,
    // second outside. Preserve NBG2 low-byte controls and sprite-window bits.
    unsigned result=0x3047;
    for (const auto pair : {std::uint32_t(33<<16|1),34u<<16|1,37u<<16,35u<<16|1,38u<<16|1})
        result=lagi::native_nbg3_window(result,pair>>16,pair&0xffff);
    if (result!=0xBE47) return 3;
    std::printf("PASS: %u production NBG3 window bit preservation cases and native MENUBK sequence (host; not window rendering)\n",cases);
}
