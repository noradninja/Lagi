#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_books_menu.h"
#include "mainMenuDebugTasks.h"
#include "3dEngine_textureCache.h"
#include "commonOverlay.h"
#include <cstring>

namespace {
constexpr u32 spriteBytes = 0x580;
constexpr u32 tileBytes = 0x16000;
constexpr u32 paletteBytes = 0x420;
constexpr u32 backupBytes = spriteBytes + tileBytes + paletteBytes;
static_assert(backupBytes == 0x169A0);
}

std::uint8_t* lagiAzelBooksBackupGraphics()
{
    // Native 06016004: contiguous backup, same three source ranges/order.
    auto* backup = dramAllocate(backupBytes);
    if (!backup) return nullptr;
    std::memcpy(backup, getVdp1Pointer(0x25C12000), spriteBytes);
    std::memcpy(backup + spriteBytes, getVdp2Vram(0x20000), tileBytes);
    std::memcpy(backup + spriteBytes + tileBytes, getVdp2Cram(0x7E0), paletteBytes);
    return backup;
}

void lagiAzelBooksRestoreGraphics(std::uint8_t* backup)
{
    if (!backup) return;
    // Native 0601604E reverses those copies, then releases the DRAM block.
    std::memcpy(getVdp1Pointer(0x25C12000), backup, spriteBytes);
    std::memcpy(getVdp2Vram(0x20000), backup + spriteBytes, tileBytes);
    std::memcpy(getVdp2Cram(0x7E0), backup + spriteBytes + tileBytes, paletteBytes);
    dramFree(backup);
    // Notify the Vita renderer of restored source data; no visual substitute.
    invalidateVdp1TextureRange(0x12000, spriteBytes);
    invalidateCramRange(0x7E0, paletteBytes);
}

std::int32_t lagiAzelBooksLookupItem(std::int32_t item)
{
    for (int index = 0; index < 22; ++index)
        if (readSaturnS16(gCommonFile->getSaturnPtr(0x20DDC8) + index * 2) == item)
            return index;
    return -1;
}

LagiAzelBooksSelection lagiAzelBooksBuildSelection(std::int32_t requestedIndex)
{
    LagiAzelBooksSelection result;
    for (int index = 0; index < 22; ++index)
    {
        const int item = readSaturnS16(gCommonFile->getSaturnPtr(0x20DDC8) + index * 2);
        if (!mainGameState.getBit(0xF3 + item)) continue;
        result.ownedIndices[result.count++] = static_cast<std::uint8_t>(index);
    }
    if (requestedIndex >= 0)
        for (int position = 0; position < result.count; ++position)
            if (result.ownedIndices[position] == requestedIndex)
            {
                result.selected = position;
                break;
            }
    return result;
}

bool lagiAzelBooksReadHeading(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t headingIndex, LagiAzelBooksHeading& out)
{
    out = {};
    if (!data || bytes < 4 || bookIndex >= 22) return false;
    auto be32 = [](const u8* p) -> u32 {
        return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | p[3];
    };
    auto range = [bytes](std::size_t offset, std::size_t length) {
        return offset <= bytes && length <= bytes - offset;
    };
    const std::size_t table = be32(data);
    if (!range(table, 22 * 16)) return false;
    const auto* book = data + table + bookIndex * 16;
    const std::size_t pages = be32(book + 4);
    // Check multiplication through the remaining image size, not wraparound.
    if (!range(pages, 8) || headingIndex > (bytes - pages - 8) / 8) return false;
    const auto* page = data + pages + std::size_t(headingIndex) * 8;
    const std::size_t title = be32(book);
    const std::size_t heading = be32(page);
    if (!range(title, 1) || !range(heading, 1) ||
        !std::memchr(data + title, 0, bytes - title) ||
        !std::memchr(data + heading, 0, bytes - heading)) return false;
    // Native 060164AC/060164EC offsets; 06016582/06016598 page metadata.
    out.title = reinterpret_cast<const char*>(data + title);
    out.heading = reinterpret_cast<const char*>(data + heading);
    out.firstBodyPage = static_cast<u16>((u16(page[4]) << 8) | page[5]);
    out.bodyPageCount = static_cast<u16>((u16(page[6]) << 8) | page[7]);
    return true;
}
