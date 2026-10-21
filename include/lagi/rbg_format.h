#pragma once
namespace lagi {
// CHSZ=16, CHCN=8bpp, bitmap disabled, one-word flip-enabled pattern names.
constexpr bool rbgTile16EightBit(unsigned chctlb,unsigned pncr) {
    return (chctlb&0x7300u)==0x1100u && (pncr&0xC000u)==0x8000u;
}
}
