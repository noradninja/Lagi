#pragma once

// Minimal Azel compile environment for Vita.
//
// Upstream Azel normally obtains these definitions through PDS.h, which also
// pulls in desktop-only SDL3/BGFX/SoLoud/ImGui/Tracy dependencies. Lagi keeps
// that platform surface out of the Vita build and supplies only the portable
// foundation required by core Azel headers.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <array>
#include <bitset>
#include <bit>
#include <vector>
#include <string>
#include <typeinfo>

using u8  = std::uint8_t;
using s8  = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;
using u64 = std::uint64_t;
using s64 = std::int64_t;

struct sSaturnMemoryFile;

struct sSaturnPtr {
    s32 m_offset = 0;
    sSaturnMemoryFile* m_file = nullptr;

    sSaturnPtr operator+(unsigned int i) const {
        sSaturnPtr p = *this;
        p.m_offset += static_cast<s32>(i);
        return p;
    }

    sSaturnPtr operator+(int i) const {
        sSaturnPtr p = *this;
        p.m_offset += i;
        return p;
    }

    bool isNull() const { return m_offset == 0; }
};

#include "fixedPoint.h"
