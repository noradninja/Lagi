#include "lagi/azel_books_menu.h"
#include <cstring>

bool lagiAzelBooksReadHeading(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t headingIndex, LagiAzelBooksHeading& out)
{
    out = {};
    if (!data || bytes < 4 || bookIndex >= 22) return false;
    auto be32 = [](const std::uint8_t* p) -> std::uint32_t {
        return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
            (std::uint32_t(p[2]) << 8) | p[3];
    };
    auto be16 = [](const std::uint8_t* p) -> std::uint16_t {
        return static_cast<std::uint16_t>((std::uint16_t(p[0]) << 8) | p[1]);
    };
    auto range = [bytes](std::size_t offset, std::size_t length) {
        return offset <= bytes && length <= bytes - offset;
    };
    const std::size_t table = be32(data);
    if (!range(table, 22 * 16)) return false;
    const auto* book = data + table + bookIndex * 16;
    if (headingIndex >= be16(book + 12)) return false;
    const std::size_t pages = be32(book + 4);
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
    out.firstBodyPage = be16(page + 4);
    out.bodyPageCount = be16(page + 6);
    return true;
}
