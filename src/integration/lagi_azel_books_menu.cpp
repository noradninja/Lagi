#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_books_menu.h"
#include "mainMenuDebugTasks.h"
#include "3dEngine_textureCache.h"
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
