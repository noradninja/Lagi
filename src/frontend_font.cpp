#include "lagi/frontend_font.h"
#include <algorithm>
#include <limits>
#include <vector>

namespace lagi::frontend {
UnpackResult unpackFont(const std::uint8_t* source,std::size_t bytes,
    std::uint8_t* destination,std::size_t capacity)
{
    UnpackResult result;
    if(!source || !destination) return result;
    unsigned control=0,bits=0;
    auto byte=[&](unsigned& value) {
        if(result.consumed>=bytes) return false;
        value=source[result.consumed++];return true;
    };
    auto bit=[&](unsigned& value) {
        if(!bits) {if(!byte(control))return false;bits=8;}
        value=control&1;control>>=1;--bits;return true;
    };
    while(true) {
        unsigned literal=0;
        if(!bit(literal))return result;
        if(literal) {
            unsigned value;
            if(!byte(value) || result.written>=capacity)return result;
            destination[result.written++]=static_cast<std::uint8_t>(value);
            continue;
        }
        unsigned longCopy=0,length=0,distance=0;
        if(!bit(longCopy))return result;
        if(longCopy) {
            unsigned low,high;
            if(!byte(low) || !byte(high))return result;
            const unsigned word=(high<<8)|low;
            if(!word) {result.ok=true;return result;}
            distance=8192-(word>>3);
            length=word&7;
            if(length)length+=2;
            else {if(!byte(length))return result;++length;}
        } else {
            unsigned a,b,offset;
            if(!bit(a) || !bit(b) || !byte(offset))return result;
            length=2+(a<<1)+b;
            distance=256-offset;
        }
        if(distance>result.written || length>capacity-result.written)return result;
        for(unsigned i=0;i<length;++i) {
            destination[result.written]=destination[result.written-distance];
            ++result.written;
        }
    }
}

bool LatinFont::load(const std::uint8_t* compressed,std::size_t bytes,
    const std::uint8_t* nativePalette32)
{
    ready_=false;decodedBytes_=0;
    if(!nativePalette32 || bytes>maximumCompressedBytes)return false;
    std::vector<std::uint8_t> decoded(maximumDecodedBytes);
    const auto result=unpackFont(compressed,bytes,decoded.data(),decoded.size());
    // printVdp2String: top tile=63hex+2*(ASCII-20hex), bottom=top+1.
    constexpr std::size_t required=(0x63+95*2)*32;
    if(!result.ok || result.written<required)return false;
    for(unsigned c=0x20;c<=0x7E;++c)for(unsigned y=0;y<16;++y)for(unsigned x=0;x<8;++x) {
        const auto tile=0x63+2*(c-0x20)+y/8;
        const auto packed=decoded[tile*32+(y&7)*4+x/2];
        glyphs_[(c-0x20)*128+y*8+x]=static_cast<std::uint8_t>(x&1?packed&15:packed>>4);
    }
    for(unsigned i=0;i<16;++i) {
        const unsigned rgb=(unsigned(nativePalette32[i*2])<<8)|nativePalette32[i*2+1];
        const auto r=(rgb&31)*255/31,g=((rgb>>5)&31)*255/31,b=((rgb>>10)&31)*255/31;
        palette_[i]=0xFF000000u|(b<<16)|(g<<8)|r;
    }
    decodedBytes_=result.written;ready_=true;return true;
}

int LatinFont::measure(const char* text)const
{
    if(!text)return 0;
    int current=0,maximum=0;
    for(;*text;++text) {
        if(*text=='\n') {maximum=std::max(maximum,current);current=0;}
        else if(current<=std::numeric_limits<int>::max()-advance)current+=advance;
        else return std::numeric_limits<int>::max();
    }
    return std::max(maximum,current);
}

std::uint8_t LatinFont::pixel(unsigned character,unsigned x,unsigned y)const
{
    if(!ready_ || x>=8 || y>=16)return 0;
    if(character<0x20 || character>0x7E)character='?';
    return glyphs_[(character-0x20)*128+y*8+x];
}

void LatinFont::draw(std::uint32_t* pixels,int surfaceWidth,int surfaceHeight,
    int pitch,int x,int y,const char* text,std::uint32_t tint)const
{
    if(!ready_ || !pixels || !text || surfaceWidth<=0 || surfaceHeight<=0 || pitch<surfaceWidth)return;
    std::array<std::uint32_t,16> colors;
    for(unsigned i=0;i<16;++i) {
        unsigned color=0;
        for(unsigned shift:{0u,8u,16u})color|=((((palette_[i]>>shift)&255)*((tint>>shift)&255))/255)<<shift;
        colors[i]=0xFF000000u|color;
    }
    std::int64_t penX=x,penY=y;
    for(const auto* p=reinterpret_cast<const unsigned char*>(text);*p;++p) {
        if(*p=='\n'){penX=x;penY+=height;continue;}
        for(unsigned gy=0;gy<16;++gy)for(unsigned gx=0;gx<8;++gx) {
            const auto dx=penX+gx,dy=penY+gy;
            if(dx<0 || dy<0 || dx>=surfaceWidth || dy>=surfaceHeight)continue;
            const auto index=pixel(*p,gx,gy);
            if(index)pixels[static_cast<std::size_t>(dy)*pitch+static_cast<std::size_t>(dx)]=colors[index];
        }
        penX+=advance;
    }
}

void LatinFont::drawScaled(std::uint32_t* pixels,int surfaceWidth,int surfaceHeight,
    int pitch,int x,int y,const char* text,int cellWidth,int cellHeight,
    std::uint32_t tint)const
{
    if(!ready_ || !pixels || !text || cellWidth<=0 || cellHeight<=0 ||
       surfaceWidth<=0 || surfaceHeight<=0 || pitch<surfaceWidth)return;
    std::array<std::uint32_t,16> colors;
    for(unsigned i=0;i<16;++i) {
        unsigned color=0;
        for(unsigned shift:{0u,8u,16u})
            color|=((((palette_[i]>>shift)&255u)*((tint>>shift)&255u))/255u)<<shift;
        colors[i]=0xFF000000u|color;
    }
    std::int64_t penX=x,penY=y;
    for(const auto* p=reinterpret_cast<const unsigned char*>(text);*p;++p) {
        if(*p=='\n'){penX=x;penY+=cellHeight;continue;}
        for(int dy=0;dy<cellHeight;++dy)for(int dx=0;dx<cellWidth;++dx) {
            const auto px=penX+dx,py=penY+dy;
            if(px<0 || py<0 || px>=surfaceWidth || py>=surfaceHeight)continue;
            const unsigned index=pixel(*p,static_cast<unsigned>(dx*8/cellWidth),
                                       static_cast<unsigned>(dy*16/cellHeight));
            if(index)pixels[static_cast<std::size_t>(py)*pitch+
                            static_cast<std::size_t>(px)]=colors[index];
        }
        penX+=cellWidth;
    }
}
}
