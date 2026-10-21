#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace lagi {
// RG32F: R retains the exact binary fixed-point value, G the native invalid
// bit. Never use half precision: it moves projected source-pixel boundaries.
struct RbgCoefficientRow {float scale=1.0f, invalid=0.0f;};
using RbgCoefficientRows=std::array<RbgCoefficientRow,256>;
inline RbgCoefficientRow decodeRbgCoefficient(const unsigned char* raw,
    unsigned address,unsigned size) {
    auto byte=[&](unsigned offset){return unsigned(raw[(address+offset)&0x7ffff]);};
    if(size==2) {
        const unsigned word=byte(0)|(byte(1)<<8);
        int value=int(word&0x7fff);
        if(value&0x4000)value-=0x8000;
        return {float(value)/1024.0f,word&0x8000?1.0f:0.0f};
    }
    const unsigned word=byte(0)|(byte(1)<<8)|(byte(2)<<16)|(byte(3)<<24);
    int value=int(word&0xffffff);
    if(value&0x800000)value-=0x1000000;
    return {float(value)/65536.0f,word&0x80000000?1.0f:0.0f};
}
inline bool buildRbgCoefficientRows(const unsigned char* raw,const float* coefficient,
    unsigned base,unsigned size,bool enabled,RbgCoefficientRows& output) {
    if(!enabled) {output.fill({1,0});return true;}
    if(!raw || (size!=2 && size!=4) || coefficient[2]!=0 ||
       !std::isfinite(coefficient[0]) || !std::isfinite(coefficient[1]) ||
       coefficient[0]<0 || coefficient[0]>65536 ||
       std::abs(coefficient[1])>32768)return false;
    RbgCoefficientRows next{};
    for(unsigned y=0;y<224;++y) {
        const double exact=double(coefficient[0])+double(coefficient[1])*y;
        // Preserve both fused and unfused FP32 shader evaluation. Reject any
        // index near a rounding boundary instead of changing its source word.
        volatile float product=coefficient[1]*float(y);
        const float separate=coefficient[0]+product;
        const float fused=std::fma(coefficient[1],float(y),coefficient[0]);
        const double index=std::floor(exact);
        if(std::floor(double(separate))!=index || std::floor(double(fused))!=index)
            return false;
        const unsigned wrapped=static_cast<unsigned>(static_cast<std::int64_t>(index))&0xffff;
        next[y]=decodeRbgCoefficient(raw,base+wrapped*size,size);
    }
    output=next;
    return true;
}
} // namespace lagi
