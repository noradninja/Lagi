#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

static inline u16 be16(const void* ptr)
{
    const u8* p = static_cast<const u8*>(ptr);
    return static_cast<u16>((static_cast<u16>(p[0]) << 8) | p[1]);
}

static inline u32 be32(const void* ptr)
{
    const u8* p = static_cast<const u8*>(ptr);
    return (static_cast<u32>(p[0]) << 24) |
           (static_cast<u32>(p[1]) << 16) |
           (static_cast<u32>(p[2]) << 8) |
           static_cast<u32>(p[3]);
}

u32 READ_BE_U32(const void* ptr) { return be32(ptr); }
s32 READ_BE_S32(const void* ptr) { return static_cast<s32>(be32(ptr)); }
u16 READ_BE_U16(const void* ptr) { return be16(ptr); }
s16 READ_BE_S16(const void* ptr) { return static_cast<s16>(be16(ptr)); }
u8 READ_BE_U8(const void* ptr) { return *static_cast<const u8*>(ptr); }
s8 READ_BE_S8(const void* ptr) { return static_cast<s8>(*static_cast<const u8*>(ptr)); }

static u32 checkedOffset(sSaturnPtr ptr, u32 bytes)
{
    assert(ptr.m_file);
    const sSaturnMemoryFile* file = ptr.m_file;
    assert(ptr.m_offset >= static_cast<s32>(file->m_base));
    const u32 offset = static_cast<u32>(ptr.m_offset) - file->m_base;
    assert(offset + bytes <= file->m_dataSize);
    return offset;
}

u8* getSaturnPtr(sSaturnPtr ptr)
{
    return ptr.m_file->m_data + checkedOffset(ptr, 1);
}

u8 readSaturnU8(sSaturnPtr ptr) { return READ_BE_U8(ptr.m_file->m_data + checkedOffset(ptr, 1)); }
s8 readSaturnS8(sSaturnPtr ptr) { return READ_BE_S8(ptr.m_file->m_data + checkedOffset(ptr, 1)); }
u16 readSaturnU16(sSaturnPtr ptr) { return READ_BE_U16(ptr.m_file->m_data + checkedOffset(ptr, 2)); }
s16 readSaturnS16(sSaturnPtr ptr) { return READ_BE_S16(ptr.m_file->m_data + checkedOffset(ptr, 2)); }
u32 readSaturnU32(sSaturnPtr ptr) { return READ_BE_U32(ptr.m_file->m_data + checkedOffset(ptr, 4)); }
s32 readSaturnS32(sSaturnPtr ptr) { return READ_BE_S32(ptr.m_file->m_data + checkedOffset(ptr, 4)); }

sSaturnPtr readSaturnEA(sSaturnPtr ptr)
{
    sSaturnPtr out = ptr;
    out.m_offset = static_cast<s32>(readSaturnU32(ptr));
    return out;
}

std::string readSaturnString(sSaturnPtr ptr)
{
    std::string out;
    if (ptr.isNull()) return out;
    while (const s8 c = readSaturnS8(ptr)) {
        out += static_cast<char>(c);
        ptr += 1;
    }
    return out;
}

fixedPoint readSaturnFP(sSaturnPtr ptr)
{
    return fixedPoint::fromS32(readSaturnS32(ptr));
}

sVec3_FP readSaturnVec3(sSaturnPtr ptr)
{
    return sVec3_FP(readSaturnS32(ptr), readSaturnS32(ptr + 4), readSaturnS32(ptr + 8));
}

void readSaturnVec3Into(sSaturnPtr src, sVec3_FP* dst)
{
    *dst = readSaturnVec3(src);
}

sVec2_S16 readSaturnVec2_S16(sSaturnPtr ptr)
{
    sVec2_S16 out{};
    out[0] = readSaturnS16(ptr);
    out[1] = readSaturnS16(ptr + 2);
    return out;
}

void memcpy_dma(void* src, void* dst, u32 size)
{
    std::memcpy(dst, src, size);
}
