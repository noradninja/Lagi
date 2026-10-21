#pragma once
#include <array>
#include <cstdint>
namespace lagi {
using Nbg0LineRows=std::array<std::array<float,2>,512>;
inline Nbg0LineRows buildNbg0LineRows(const std::uint8_t* raw,const float s[8][4]) {
    Nbg0LineRows rows{};
    const auto word=[&](unsigned a){return unsigned(raw[a&0x7ffff])*256u+raw[(a+1)&0x7ffff];};
    const auto fixed=[&](unsigned a){unsigned hi=word(a);return float(hi>=32768?int(hi)-65536:int(hi))+word(a+2)/65536.0f;};
    unsigned ctl=unsigned(s[3][1]);
    unsigned stride=4u*(((ctl&2)?1u:0u)+((ctl&4)?1u:0u)+((ctl&8)?1u:0u));
    const unsigned interval=s[3][2]>=1 ? unsigned(s[3][2]) : 1u;
    for(unsigned y=0;y<224;++y){
        unsigned a=unsigned(s[3][0])+(y/interval)*stride;
        float x=0,py=y*s[2][3],z=s[2][2];
        if(ctl&2){x=fixed(a);a+=4;}
        if(ctl&4){py=fixed(a)+float(y%interval);a+=4;}
        if(ctl&8)z=fixed(a);
        rows[y]={x,py};rows[256+y]={z,0};
    }
    return rows;
}
}
