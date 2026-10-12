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

bool lagiAzelBooksReadBodyPage(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t bodyPageIndex, LagiAzelBooksBodyPage& out)
{
    out = {};
    if (!data || bytes < 4 || bookIndex >= 22) return false;
    auto be32 = [](const std::uint8_t* p) -> std::uint32_t {
        return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
            (std::uint32_t(p[2]) << 8) | p[3];
    };
    auto range = [bytes](std::size_t offset, std::size_t length) {
        return offset <= bytes && length <= bytes - offset;
    };
    const std::size_t table = be32(data);
    if (!range(table, 22 * 16)) return false;
    const auto* book = data + table + bookIndex * 16;
    const auto count = (std::uint32_t(book[14]) << 8) | book[15];
    if (bodyPageIndex >= count) return false;
    const std::size_t pages = be32(book + 8);
    if (!range(pages, 4) || bodyPageIndex > (bytes - pages - 4) / 4) return false;
    const std::uint32_t value = be32(data + pages + std::size_t(bodyPageIndex) * 4);
    // Native 060165D4 compares the table value to 3 before dereferencing.
    // 1/2 select illustration setup; 0/3 leave the illustration disabled.
    if (value > 3) {
        if (!range(value, 1) || !std::memchr(data + value, 0, bytes - value)) return false;
        out.text = reinterpret_cast<const char*>(data + value);
    }
    out.nativeValue = value;
    return true;
}

bool lagiAzelBooksHeadingForBodyPage(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t bodyPageIndex, std::uint32_t& headingIndex)
{
    headingIndex = 0;
    LagiAzelBooksBodyPage body;
    if (!lagiAzelBooksReadBodyPage(data, bytes, bookIndex, bodyPageIndex, body)) return false;
    // ReadHeading validates the record table and each logical heading index.
    const std::size_t table = (std::uint32_t(data[0]) << 24) |
        (std::uint32_t(data[1]) << 16) | (std::uint32_t(data[2]) << 8) | data[3];
    const auto* book = data + table + bookIndex * 16;
    const auto count = (std::uint32_t(book[12]) << 8) | book[13];
    for (std::uint32_t index = 0; index < count; ++index) {
        LagiAzelBooksHeading heading;
        if (!lagiAzelBooksReadHeading(data, bytes, bookIndex, index, heading)) return false;
        if (bodyPageIndex >= heading.firstBodyPage &&
            bodyPageIndex - heading.firstBodyPage < heading.bodyPageCount) {
            headingIndex = index;
            return true;
        }
    }
    return true;
}

LagiAzelBooksIndexStep lagiAzelBooksStepIndex(std::int32_t& index,
    std::int32_t minimum, std::int32_t maximum, std::uint16_t repeated,
    std::uint16_t held, bool vertical, bool& boundarySoundLatched)
{
    const std::uint16_t decrement = vertical ? 0x10 : 0x40;
    const std::uint16_t increment = vertical ? 0x20 : 0x80;
    const auto mask = decrement | increment;
    if (!(repeated & mask)) {
        if (!(held & mask)) boundarySoundLatched = false;
        return {};
    }
    // Keep the native branch ordering: decrement wins if valid. If it
    // underflows, the increment branch uses that decremented candidate.
    std::int64_t candidate = index;
    bool accepted = false;
    if (repeated & decrement) {
        --candidate;
        accepted = candidate >= minimum;
    }
    if (!accepted && (repeated & increment)) {
        ++candidate;
        accepted = candidate <= maximum;
    }
    if (accepted) {
        index = static_cast<std::int32_t>(candidate);
        boundarySoundLatched = false;
        return {true, 0xA};
    }
    const int sound = boundarySoundLatched ? -1 : 5;
    boundarySoundLatched = true;
    return {false, sound};
}

LagiAzelBooksNavigationResult lagiAzelBooksNavigate(
    const std::uint8_t* data, std::size_t bytes,
    const LagiAzelBooksSelection& owned, LagiAzelBooksNavigation& navigation,
    std::uint16_t pressed, std::uint16_t repeated, std::uint16_t held)
{
    LagiAzelBooksNavigationResult result;
    // 06016740..4E: cancel precedes all focus/index processing.
    if (pressed & 1) { result.valid = true; result.cancel = true; return result; }
    if (!data || bytes < 4 || owned.count > owned.ownedIndices.size() ||
        navigation.focus < 0 || navigation.focus > 2) return result;
    const std::size_t table = (std::uint32_t(data[0]) << 24) |
        (std::uint32_t(data[1]) << 16) | (std::uint32_t(data[2]) << 8) | data[3];
    if (table > bytes || bytes - table < 22 * 16) return result;
    auto countFor = [&](int focus) -> int {
        if (focus == 0) return owned.count;
        const int position = navigation.indices[0];
        if (position < 0 || position >= owned.count) return -1;
        const unsigned book = owned.ownedIndices[position];
        if (book >= 22) return -1;
        const auto* record = data + table + book * 16;
        const int offset = focus == 1 ? 12 : 14;
        return (unsigned(record[offset]) << 8) | record[offset + 1];
    };
    // Work on a copy: malformed resources must not partially mutate input.
    auto next = navigation;
    result.sounds[0] = lagiAzelBooksStepIndex(next.focus, 0, 2, repeated,
        held, true, next.verticalBoundary).sound;
    const int count = countFor(next.focus);
    if (count < 0) return {};
    result.valid = true;
    if (!count) { navigation = next; return result; } // native 060167A4
    auto& index = next.indices[next.focus];
    if (index < 0 || index >= count) return {};
    const auto step = lagiAzelBooksStepIndex(index, 0, count - 1, repeated,
        held, false, next.horizontalBoundary);
    result.sounds[1] = step.sound;
    if (step.accepted) {
        result.reveal = true; // state 5, even for accepted two-direction no-op
        const unsigned book = owned.ownedIndices[next.indices[0]];
        if (next.focus == 0) next.indices[1] = 0;
        if (next.focus < 2) {
            LagiAzelBooksHeading heading;
            if (!lagiAzelBooksReadHeading(data, bytes, book, next.indices[1], heading)) return {};
            next.indices[2] = heading.firstBodyPage;
        } else {
            std::uint32_t heading;
            if (!lagiAzelBooksHeadingForBodyPage(data, bytes, book, next.indices[2], heading)) return {};
            next.indices[1] = static_cast<std::int32_t>(heading);
        }
    }
    // 06016806..3A uses the pre-change logical maximum, preserving native order.
    result.updateCursor = true;
    result.canDecrease = index != 0;
    result.canIncrease = index < count - 1;
    navigation = next;
    return result;
}

bool lagiAzelBooksCalculateWindow(std::int32_t scrollX, std::int32_t scrollY,
    std::int32_t step, std::int32_t duration, LagiAzelBooksWindow& out)
{
    if (duration <= 0 || step < 0 || step > duration) return false;
    // 06035686 performs signed 64-bit multiplication/division, truncating
    // toward zero. Do not round the fractional reveal to nearest pixel.
    const auto halfWidth = static_cast<std::int32_t>(std::int64_t(0x88) * step / duration);
    const auto halfHeight = static_cast<std::int32_t>(std::int64_t(0x38) * step / duration);
    const auto centerX = 0xB0 - scrollX;
    const auto centerY = 0x1A8 - scrollY;
    out.left = centerX - halfWidth;
    out.right = centerX + halfWidth;
    out.top = centerY - halfHeight;
    out.bottom = centerY + halfHeight;
    out.registerLeft = out.left * 2;
    out.registerRight = out.right * 2 - 1;
    out.registerTop = out.top;
    out.registerBottom = out.bottom - 1;
    out.quad = {out.left - 0xB4, 0x73 - out.top,
        out.right - 0xAD, 0x73 - out.top,
        out.right - 0xAD, 0x6E - out.bottom,
        out.left - 0xB4, 0x6E - out.bottom};
    return true;
}
