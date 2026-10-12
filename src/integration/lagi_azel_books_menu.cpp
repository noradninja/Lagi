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

void drawGaugeVdp1(u16 mode, std::array<sVec2_S16, 4>& params, u16 color, fixedPoint depth);
void scrollMenu();

bool lagiAzelBooksPresentText(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t headingIndex,
    std::uint32_t bodyPageIndex, LagiAzelBooksBodyPage& body)
{
    LagiAzelBooksHeading heading;
    body = {};
    if (!lagiAzelBooksReadHeading(data, bytes, bookIndex, headingIndex, heading) ||
        !lagiAzelBooksReadBodyPage(data, bytes, bookIndex, bodyPageIndex, body)) return false;
    // 060164AC / 060164EC: native glyph coordinates, not output pixels.
    setupVDP2StringRendering(7, 0x24, 0x1E, 2);
    clearVdp2TextArea();
    vdp2StringContext.m0 = 0;
    drawObjectName(heading.title);
    setupVDP2StringRendering(7, 0x27, 0x1E, 2);
    clearVdp2TextArea();
    vdp2StringContext.m0 = 0;
    drawObjectName(heading.heading);
    // 0601653E: one-based page within this heading / heading page count.
    setupVDP2StringRendering(7, 0x2A, 6, 2);
    clearVdp2TextArea();
    vdp2StringContext.m0 = 0;
    printVdp2Number2(static_cast<int>(bodyPageIndex) - heading.firstBodyPage + 1, 1);
    // Original MAIN literal 06043244 is 80 82 00, an extended font glyph.
    // ASCII '/' would bypass the native MENUBK font mapping.
    printVdp2String2("\x80\x82");
    printVdp2Number2(heading.bodyPageCount, 1);
    // 060165D4 clears this region for text AND illustration commands.
    setupVDP2StringRendering(5, 0x2E, 0x22, 0xE);
    clearVdp2TextArea();
    // Original 060165D4 clears only NBG2's priority bits for text/commands
    // 0 and 3; commands 1/2 select the two existing NBG2 illustrations.
    // Keep the pending bank/dirty handoff, rather than forcing both banks.
    auto* regs = vdp2Controls.m4_pendingVdp2Regs;
    if (body.nativeValue == 1 || body.nativeValue == 2) {
        setVdp2LayerScroll(2, body.nativeValue == 1 ? -0x68 : 0x98, 0x100);
        scrollMenu(); // native 06015B08 publishes all four configured layers
        regs->mFA_PRINB = (regs->mFA_PRINB & 0xFFF8) | 5;
    } else {
        regs->mFA_PRINB &= 0xFFF8;
    }
    vdp2Controls.m_isDirty = 1;
    if (body.text) {
        vdp2StringContext.m0 = 0;
        drawObjectName(body.text);
    }
    return true;
}

bool lagiAzelBooksPresentWindow(std::int32_t scrollX, std::int32_t scrollY,
    std::int32_t step, std::int32_t duration)
{
    LagiAzelBooksWindow window{};
    if (!lagiAzelBooksCalculateWindow(scrollX, scrollY, step, duration, window)) return false;
    // 06016088 writes both banks at offsets C8/CA/CC/CE, not window 0.
    for (auto& regs : vdp2Controls.m20_registers) {
        regs.mC8_WPSX1 = static_cast<u16>(window.registerLeft);
        regs.mCA_WPSY1 = static_cast<u16>(window.registerTop);
        regs.mCC_WPEX1 = static_cast<u16>(window.registerRight);
        regs.mCE_WPEY1 = static_cast<u16>(window.registerBottom);
    }
    std::array<sVec2_S16, 4> quad;
    for (unsigned i = 0; i < 4; ++i) {
        quad[i][0] = static_cast<s16>(window.quad[i * 2]);
        quad[i][1] = static_cast<s16>(window.quad[i * 2 + 1]);
    }
    // Native 0602CB5E is drawGaugeVdp1: preserve C0 mode, D294 color,
    // zero sort depth and native Y inversion inside that shared helper.
    drawGaugeVdp1(0xC0, quad, 0xD294, 0);
    return true;
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
