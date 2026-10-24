#pragma once
#include <cstdint>
#include <cstddef>
namespace lagi {
inline std::uint32_t nativeColorWord(const std::uint8_t* p) {
    return std::uint32_t(p[0])<<24 | std::uint32_t(p[1])<<16 |
           std::uint32_t(p[2])<<8 | p[3];
}
// MCB color stream offsets are relative to the stream, not the bundle.
// MAIN 060206DE/0602073C: frame -> preorder bone -> sparse polygon records.
inline const std::uint8_t* nativeColorBone(const std::uint8_t* stream,
    int frame, unsigned bone) {
    if(!stream || frame<0 || unsigned(frame)>=(unsigned(stream[0])<<8|stream[1]))return nullptr;
    const auto table=nativeColorWord(stream+4);
    if(!table)return nullptr;
    const auto bones=nativeColorWord(stream+table+unsigned(frame)*4);
    if(!bones)return nullptr;
    const auto records=nativeColorWord(stream+bones+bone*4);
    return records ? stream+records : nullptr;
}
struct NativeAnimatedPolygonColor {
    std::uint32_t polygon;
    std::int8_t rgb[4][3];
};
// MAIN 0601E83C..0601EAD6: sorted polygon index followed by four BGR byte
// triples. Bytes are signed 8.8 additive material channels before clamp, not
// RGB555 colors, alpha, or a replacement for the normal/directional lighting.
template<class Emit> inline bool nativeColorRecords(const std::uint8_t* records,
    std::size_t polygonCount, Emit emit) {
    if(!records)return true;
    std::uint32_t previous=0;
    bool first=true;
    for(std::size_t n=0;n<=polygonCount;++n,records+=16) {
        const auto index=nativeColorWord(records);
        if(index==0xffffffffu)return true;
        if(index>=polygonCount || (!first && index<=previous))return false;
        NativeAnimatedPolygonColor color{};color.polygon=index;
        for(unsigned c=0;c<4;++c)for(unsigned axis=0;axis<3;++axis)
            color.rgb[c][axis]=static_cast<std::int8_t>(records[4+c*3+2-axis]);
        emit(color);previous=index;first=false;
    }
    return false;
}
template<class Record> inline void applyNativeAnimatedColor(Record& record,
    const NativeAnimatedPolygonColor& color) {
    const unsigned mode=(record.lightingControl>>8)&3u;
    if(mode==0 || record.lightingCount==0)return;
    // Single-normal polygons still receive four independent color corners.
    if(mode==1)for(unsigned c=1;c<4;++c)record.lighting[c]=record.lighting[0];
    record.lightingControl=(record.lightingControl&~0x300u)|0x200u;
    record.lightingCount=4;
    for(unsigned c=0;c<4;++c) {
        record.lighting[c].hasColor=true;
        for(unsigned axis=0;axis<3;++axis)
            record.lighting[c].color[axis]=static_cast<std::uint16_t>(
                int(color.rgb[c][axis])*256);
    }
}
}
