#pragma once
#include <cstdint>
namespace lagi {
// Native 0605AF6C: selector is the old byte value before postincrement.
inline std::uint32_t garilSinglePositionTable(unsigned selector)
{
    return 0x060AF90Cu+(selector==0 ? 0x48u : selector==1 ? 0x90u : 0u);
}
}
