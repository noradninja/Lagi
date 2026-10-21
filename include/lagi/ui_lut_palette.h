#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace lagi {
using UiLutPalette = std::array<std::uint16_t,16>;
// Capture the colors the native 4bpp LUT decoder will actually consume.
// LUT words are big-endian; bit 15 selects direct RGB555, otherwise CRAM.
inline bool capture_ui_lut_palette(const std::uint8_t* lut,
    const std::uint8_t* cram,std::size_t cramBytes,UiLutPalette& out)
{
    if (!lut || !cram) return false;
    for (unsigned i=0;i<16;++i) {
        const std::uint16_t value=(std::uint16_t(lut[i*2])<<8)|lut[i*2+1];
        if (value&0x8000u) out[i]=value;
        else {
            const unsigned offset=(value&0x7ffu)*2u;
            if (offset+1>=cramBytes) return false;
            out[i]=(std::uint16_t(cram[offset])<<8)|cram[offset+1];
        }
    }
    return true;
}
inline void refresh_ui_texture_rows(std::uint32_t* destination,
    const std::uint32_t* source,unsigned width,unsigned height)
{
    const unsigned stride=(width+7u)&~7u;
    for (unsigned y=0;y<height;++y)
        for (unsigned x=0;x<width;++x)
            destination[y*stride+x]=source[y*width+x];
}
} // namespace lagi
