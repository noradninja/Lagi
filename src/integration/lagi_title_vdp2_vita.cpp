#include "lagi/platform.h"
#include "lagi/lagi_azel_upstream_prelude.h"
#include "VDP2.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace lagi::azel {
namespace {

constexpr int kLogicalWidth = 352;
constexpr int kLogicalHeight = 224;
constexpr int kOutputWidth = 480;
constexpr int kOutputHeight = 272;

static std::array<std::uint32_t,
                  static_cast<std::size_t>(kOutputWidth) *
                  static_cast<std::size_t>(kOutputHeight)> g_titleFrame{};

static std::uint16_t readBe16(const unsigned char* p)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(p[0]) << 8) |
        static_cast<std::uint16_t>(p[1]));
}

static std::uint32_t rgb555ToAbgr(std::uint16_t color)
{
    const std::uint8_t r = static_cast<std::uint8_t>((color & 0x1Fu) << 3);
    const std::uint8_t g = static_cast<std::uint8_t>(((color >> 5) & 0x1Fu) << 3);
    const std::uint8_t b = static_cast<std::uint8_t>(((color >> 10) & 0x1Fu) << 3);
    return 0xFF000000u |
           static_cast<std::uint32_t>(r) |
           (static_cast<std::uint32_t>(g) << 8) |
           (static_cast<std::uint32_t>(b) << 16);
}

static std::uint32_t sampleTitleNbg0(
    const unsigned char* vram,
    const unsigned char* cram,
    int logicalX,
    int logicalY)
{
    // TITLE_OVERLAY sets 704-dot horizontal resolution and double-density
    // interlace. Treat 352x224 as the logical Saturn image and sample the
    // physical NBG0 at 2x in each axis.
    int x = logicalX * 2;
    int y = logicalY * 2;

    // TITLEE.PNB: plane A at 0x10000, B at 0x10800, then A/B repeated for
    // planes C/D. The visible 704-dot line spans A plus the left part of B.
    const std::size_t planeOffset =
        x < 512 ? 0x10000u : 0x10800u;
    x &= 511;
    y &= 511;

    const int patternX = x >> 4;
    const int patternY = y >> 4;
    const std::size_t patternAddress =
        planeOffset +
        static_cast<std::size_t>(
            (patternY * 32 + patternX) * 2);

    const std::uint16_t patternName =
        readBe16(vram + patternAddress);

    // Exact Azel VDP2 rules for PNB=1, CHSZ=1, CHCN=1, CNSM=1, SCN=0.
    const std::uint32_t characterNumber =
        static_cast<std::uint32_t>(patternName & 0x0FFFu) << 2;
    const std::size_t characterOffset =
        static_cast<std::size_t>(characterNumber) * 0x20u;

    const int px = x & 15;
    const int py = y & 15;
    const int cellX = px >> 3;
    const int cellY = py >> 3;
    const int cellIndex = cellX + cellY * 2;
    const std::size_t cellOffset =
        characterOffset +
        static_cast<std::size_t>(cellIndex) * 64u;

    const std::uint8_t colorIndex =
        vram[cellOffset +
             static_cast<std::size_t>(py & 7) * 8u +
             static_cast<std::size_t>(px & 7)];
    if (!colorIndex)
        return 0xFF000000u;

    const unsigned int paletteNumber =
        (patternName >> 8) & 0x70u;
    const unsigned int paletteEntry =
        (paletteNumber << 4) |
        static_cast<unsigned int>(colorIndex);
    const std::size_t cramOffset =
        static_cast<std::size_t>(paletteEntry) * 2u;
    if (cramOffset + 1u >= 0x1000u)
        return 0xFF000000u;

    return rgb555ToAbgr(readBe16(cram + cramOffset));
}

static std::uint32_t sampleTitleNbg1(
    const unsigned char* vram,
    const unsigned char* cram,
    int logicalX,
    int logicalY)
{
    // Title NBG1 is Azel's ordinary VDP2 text plane: 64x64 8x8 patterns,
    // 4bpp, PNB=1, CNSM=1, CAOS=7, map base 0x6000.
    if (logicalX < 0 || logicalY < 0 ||
        logicalX >= 512 || logicalY >= 512)
        return 0u;

    const int tileX = logicalX >> 3;
    const int tileY = logicalY >> 3;
    const std::size_t mapAddress =
        0x6000u +
        static_cast<std::size_t>(
            (tileY * 64 + tileX) * 2);
    const std::uint16_t patternName =
        readBe16(vram + mapAddress);
    if (!patternName)
        return 0u;

    const std::uint32_t characterNumber =
        static_cast<std::uint32_t>(patternName & 0x0FFFu);
    const std::size_t characterOffset =
        static_cast<std::size_t>(characterNumber) * 0x20u;

    const int px = logicalX & 7;
    const int py = logicalY & 7;
    const std::size_t dotOffset =
        characterOffset +
        static_cast<std::size_t>(py * 4 + (px >> 1));
    const std::uint8_t packed = vram[dotOffset];
    const unsigned int colorIndex =
        (px & 1)
            ? static_cast<unsigned int>(packed & 0x0Fu)
            : static_cast<unsigned int>(packed >> 4);
    if (!colorIndex)
        return 0u;

    const unsigned int paladdr =
        (patternName & 0xF000u) >> 8;
    const unsigned int paletteEntry =
        7u * 0x100u |
        (paladdr | colorIndex);
    const std::size_t cramOffset =
        static_cast<std::size_t>(paletteEntry) * 2u;
    if (cramOffset + 1u >= 0x1000u)
        return 0u;

    return rgb555ToAbgr(readBe16(cram + cramOffset));
}

} // namespace

bool present_native_title_vdp2()
{
    const unsigned char* const vram = getVdp2Vram(0);
    const unsigned char* const cram = getVdp2Cram(0);
    if (!vram || !cram)
        return false;

    for (int oy = 0; oy < kOutputHeight; ++oy) {
        const int ly = std::min(
            kLogicalHeight - 1,
            (oy * kLogicalHeight) / kOutputHeight);
        for (int ox = 0; ox < kOutputWidth; ++ox) {
            const int lx = std::min(
                kLogicalWidth - 1,
                (ox * kLogicalWidth) / kOutputWidth);

            std::uint32_t pixel =
                sampleTitleNbg0(vram, cram, lx, ly);
            const std::uint32_t text =
                sampleTitleNbg1(vram, cram, lx, ly);
            if (text)
                pixel = text;

            g_titleFrame[
                static_cast<std::size_t>(oy) * kOutputWidth +
                static_cast<std::size_t>(ox)] = pixel;
        }
    }

    return platform::renderer::movie_present_frame(
        g_titleFrame.data(),
        kOutputWidth,
        kOutputHeight,
        kOutputWidth);
}

} // namespace lagi::azel
