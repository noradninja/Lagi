#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace lagi {
// Native calculations remain host integers. Serialize only at the DMA boundary,
// so GPU/VDP2 readers see Saturn-order words without changing the task's math.
inline void serializeNativeLineTable(const void* source,std::uint8_t* dest,std::size_t words) {
    auto* bytes=static_cast<const std::uint8_t*>(source);
    for(std::size_t i=0;i<words;++i) {
        std::uint32_t word;
        std::memcpy(&word,bytes+i*4,4);
        dest[i*4]=std::uint8_t(word>>24);dest[i*4+1]=std::uint8_t(word>>16);
        dest[i*4+2]=std::uint8_t(word>>8);dest[i*4+3]=std::uint8_t(word);
    }
}
}
