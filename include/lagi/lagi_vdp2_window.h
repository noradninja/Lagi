#pragma once
#include <cstdint>
namespace lagi {
// Native setupNBG3 06033864, dispatch table 060338C0. Commands 33..39
// select window logic, enables and inside/outside controls in WCTLB high byte.
inline std::uint16_t native_nbg3_window(std::uint16_t prior,
    unsigned command, std::uint32_t value)
{
    const unsigned shifts[7]={15,9,11,13,8,10,12};
    if (command<33 || command>39) return prior;
    const unsigned shift=shifts[command-33];
    return static_cast<std::uint16_t>((prior & ~(1u<<shift)) | (value<<shift));
}
}
