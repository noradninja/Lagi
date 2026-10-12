#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

struct LagiAzelMapDescriptor {
    std::uint32_t nativeAddress = 0;
    const char* mapFile = nullptr;
    const char* patternFile = nullptr;
    // Preserve original bytes +8..B; interpretation belongs to native task.
    std::array<std::uint8_t,4> presentation{};
};

// Original 06016D36..46: COMMON table 0020E848, 23 field pointers, then
// subfield pointers to 12-byte descriptors. Caller supplies the authoritative
// current field's subfield count; never infer it from adjacent COMMON data.
// Returned strings borrow COMMON's lifetime. No disc resources are copied.
inline bool lagiAzelReadFieldMapDescriptor(const std::uint8_t* common,
    std::size_t bytes, std::uint32_t field, std::uint32_t subfield,
    std::uint32_t subfieldCount, LagiAzelMapDescriptor& out)
{
    out = {};
    constexpr std::uint32_t base=0x200000, table=0xE848;
    if (!common || field>=23 || subfield>=subfieldCount ||
        bytes<table || bytes-table<23*4) return false;
    const auto be32=[](const std::uint8_t* p) {
        return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|
            (std::uint32_t(p[2])<<8)|p[3];
    };
    const auto range=[&](std::uint32_t address,std::size_t length) {
        return address>=base && std::size_t(address-base)<=bytes &&
            length<=bytes-std::size_t(address-base);
    };
    const std::uint32_t subtable=be32(common+table+field*4);
    if (!range(subtable,4) || std::uint64_t(subfield)*4>bytes-(subtable-base)-4) return false;
    const std::uint32_t descriptor=be32(common+(subtable-base)+std::size_t(subfield)*4);
    if (!range(descriptor,12)) return false;
    const auto* record=common+(descriptor-base);
    const std::uint32_t map=be32(record), pattern=be32(record+4);
    if (!range(map,1) || !range(pattern,1) ||
        !std::memchr(common+(map-base),0,bytes-(map-base)) ||
        !std::memchr(common+(pattern-base),0,bytes-(pattern-base))) return false;
    out.nativeAddress=descriptor;
    out.mapFile=reinterpret_cast<const char*>(common+(map-base));
    out.patternFile=reinterpret_cast<const char*>(common+(pattern-base));
    std::copy(record+8,record+12,out.presentation.begin());
    return true;
}
