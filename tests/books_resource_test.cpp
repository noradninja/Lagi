#include "lagi/azel_books_menu.h"
#include <cstdio>
#include <vector>
#include <algorithm>

// Read-only test of the production reader against the user's Disc 1 resource.
int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    FILE* disc = nullptr;
    if (fopen_s(&disc, argv[1], "rb") || !disc) return 3;
    std::vector<std::uint8_t> data(34988);
    for (std::size_t offset = 0; offset < data.size(); offset += 2048)
    {
        const auto bytes = std::min<std::size_t>(2048, data.size() - offset);
        if (_fseeki64(disc, (216991LL + offset / 2048) * 2352 + 16, SEEK_SET) ||
            fread(data.data() + offset, 1, bytes, disc) != bytes)
        { fclose(disc); return 4; }
    }
    fclose(disc);
    auto be16 = [&](std::size_t p) { return unsigned(data[p]) * 256 + data[p+1]; };
    const unsigned table = unsigned(data[0])*16777216 + unsigned(data[1])*65536 +
        unsigned(data[2])*256 + data[3];
    if (table != 0x874C || table + 22*16 > data.size()) return 5;
    unsigned tested = 0;
    unsigned bodyTested = 0;
    unsigned illustrations = 0;
    LagiAzelBooksHeading heading;
    LagiAzelBooksBodyPage body;
    for (unsigned book = 0; book < 22; ++book)
    {
        const unsigned count = be16(table + book*16 + 12);
        const unsigned bodyCount = be16(table + book*16 + 14);
        for (unsigned page = 0; page < count; ++page)
        {
            if (!lagiAzelBooksReadHeading(data.data(), data.size(), book, page, heading) ||
                !heading.title || !heading.heading ||
                heading.firstBodyPage + heading.bodyPageCount > bodyCount) return 6;
            ++tested;
        }
        if (lagiAzelBooksReadHeading(data.data(), data.size(), book, count, heading)) return 7;
        for (unsigned page = 0; page < bodyCount; ++page) {
            if (!lagiAzelBooksReadBodyPage(data.data(), data.size(), book, page, body) ||
                ((body.nativeValue > 3) != (body.text != nullptr))) return 9;
            ++bodyTested;
            unsigned selectedHeading = 0;
            if (!lagiAzelBooksHeadingForBodyPage(data.data(), data.size(), book, page, selectedHeading) ||
                !lagiAzelBooksReadHeading(data.data(), data.size(), book, selectedHeading, heading) ||
                page < heading.firstBodyPage || page - heading.firstBodyPage >= heading.bodyPageCount) return 11;
            if (body.nativeValue == 1 || body.nativeValue == 2) ++illustrations;
        }
        if (lagiAzelBooksReadBodyPage(data.data(), data.size(), book, bodyCount, body)) return 10;
    }
    if (lagiAzelBooksReadHeading(data.data(), data.size(), 22, 0, heading) ||
        lagiAzelBooksReadHeading(data.data(), 4, 0, 0, heading) ||
        lagiAzelBooksReadHeading(nullptr, data.size(), 0, 0, heading)) return 8;
    printf("PASS: production reader, 22 real books / %u headings; invalid book, heading and truncated image rejected (host, not Vita)\n", tested);
    printf("PASS: %u native body pages, including %u illustration commands (host, not renderer execution)\n", bodyTested, illustrations);
    printf("PASS: body-page-to-heading selection for all %u real pages (host, not input execution)\n", bodyTested);
    return 0;
}
