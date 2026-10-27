#pragma once
namespace lagi {
// CHSZ=16, CHCN=8bpp, bitmap disabled, one-word flip-enabled pattern names.
constexpr bool rbgTile16EightBit(unsigned chctlb,unsigned pncr) {
    return (chctlb&0x7300u)==0x1100u && (pncr&0xC000u)==0x8000u;
}
// Uniform fields consumed by the specialized RBG0 shader. Keep this native
// register decode shared with the host fixture rather than duplicating it.
struct RbgSpecializedFormat {
    unsigned scn;
    unsigned caos;
    unsigned plsz;
    unsigned screenOverAndR0tpon;
};
constexpr RbgSpecializedFormat rbgSpecializedFormat(
    unsigned pncr, unsigned craofb, unsigned packed) {
    return {pncr & 1023u, craofb & 7u, packed & 3u,
        ((packed >> 2u) & 3u) | ((packed & 16u) ? 4u : 0u)};
}
}
