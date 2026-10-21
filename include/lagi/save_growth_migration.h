#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace lagi::save {
inline std::uint32_t word(const void* p) {std::uint32_t n;std::memcpy(&n,p,4);return n;}
inline std::uint32_t checksum(const std::uint8_t* p,std::size_t size) {
    std::uint32_t sum=0;for(std::size_t i=4;i<size;++i)sum+=static_cast<std::uint32_t>(i-3)*p[i];return sum;
}
// Validate before moving any bytes. The disk file is never modified on read.
inline bool expand_growth(std::uint8_t* data,std::size_t stored,std::size_t capacity,std::size_t at) {
    constexpr std::size_t extension=48;
    if(stored<8||stored>capacity||word(data)!=checksum(data,stored))return false;
    const auto version=word(data+4);
    if(version==0x10001)return stored==capacity;
    if(version!=0x10000||capacity-stored!=extension||at<8||at>stored)return false;
    std::memmove(data+at+extension,data+at,stored-at);
    std::memset(data+at,0,extension);
    const std::uint32_t current=0x10001;std::memcpy(data+4,&current,4);
    const auto sum=checksum(data,capacity);std::memcpy(data,&sum,4);return true;
}
}
