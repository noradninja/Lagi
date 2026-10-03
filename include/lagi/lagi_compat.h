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

    sSaturnPtr& operator++() {
        ++m_offset;
        return *this;
    }

    sSaturnPtr operator++(int) {
        sSaturnPtr result(*this);
        ++(*this);
        return result;
    }

    sSaturnPtr& operator--() {
        --m_offset;
        return *this;
    }

    sSaturnPtr operator--(int) {
        sSaturnPtr result(*this);
        --(*this);
        return result;
    }

    sSaturnPtr& operator+=(unsigned int i) {
        m_offset += static_cast<s32>(i);
        return *this;
    }

    sSaturnPtr operator-(unsigned int i) const {
        sSaturnPtr p = *this;
        p.m_offset -= static_cast<s32>(i);
        return p;
    }

    sSaturnPtr& operator-=(unsigned int i) {
        m_offset -= static_cast<s32>(i);
        return *this;
    }

    bool operator==(const sSaturnPtr& other) const {
        return m_offset == other.m_offset && m_file == other.m_file;
    }

    bool operator!=(const sSaturnPtr& other) const {
        return !(*this == other);
    }

    bool isNull() const { return m_offset == 0; }

    static sSaturnPtr& getNull() {
        static sSaturnPtr temp{};
        temp.m_offset = 0;
        temp.m_file = nullptr;
        return temp;
    }

    static sSaturnPtr& createFromRaw(u32 offset, sSaturnMemoryFile* file = nullptr) {
        static sSaturnPtr temp{};
        temp.m_offset = static_cast<s32>(offset);
        temp.m_file = file;
        return temp;
    }
};

struct sSaturnMemoryFile {
    sSaturnMemoryFile() = default;
    sSaturnMemoryFile(const char* fileName, u32 base = 0x06054000);

    std::string m_name;
    u8* m_data = nullptr;
    u32 m_dataSize = 0;
    u32 m_base = 0;

    sSaturnPtr getSaturnPtr(u32 address) {
        sSaturnPtr p;
        p.m_file = this;
        p.m_offset = static_cast<s32>(address);
        return p;
    }
};

#include "fixedPoint.h"

// Azel's task templates reference logging category names even in shipping
// builds. Upstream defines these in PDS_Logger.h, but that header depends on
// ImGui. Preserve only the category API required by portable core headers.
enum eLogCategories {
    log_default = 0,
    log_task,
    log_unimlemented,
    log_m68k,
    log_warning,
    log_max
};

#ifndef PDS_Log
#define PDS_Log(...)
#endif
#ifndef PDS_CategorizedLog
#define PDS_CategorizedLog(...)
#endif
#ifndef PDS_unimplemented
#define PDS_unimplemented(...)
#endif
#ifndef PDS_warningOnce
#define PDS_warningOnce(...)
#endif
