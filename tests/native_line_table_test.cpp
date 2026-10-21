#include "lagi/native_line_table.h"
#include <array>
#include <cassert>
#include <cstdio>
int main() {
    std::array<std::uint32_t,224*3> source{};
    std::array<std::uint8_t,0xC00> dest{};
    for(unsigned y=0;y<224;++y) {
        source[y*3]=std::uint32_t(-int(y*65536));
        source[y*3+1]=y*65536;source[y*3+2]=0x10000;
    }
    const auto original=source;
    lagi::serializeNativeLineTable(source.data(),dest.data(),source.size());
    assert(source==original);
    for(unsigned i=0;i<source.size();++i) {
        const unsigned p=i*4;
        const auto decoded=(std::uint32_t(dest[p])<<24)|(std::uint32_t(dest[p+1])<<16)|
            (std::uint32_t(dest[p+2])<<8)|dest[p+3];
        assert(decoded==source[i]);
    }
    for(unsigned i=source.size()*4;i<dest.size();++i)assert(dest[i]==0);
    puts("PASS 224 native line rows, signed scroll, zoom, byte order and source preservation");
}
