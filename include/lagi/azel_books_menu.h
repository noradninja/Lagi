#pragma once
#include <cstdint>
#include <array>
#include <cstddef>

// Native 06016004/0601604E resource ownership boundary. The caller owns the
// returned DRAM block until restore; the Books task itself is not ported yet.
std::uint8_t* lagiAzelBooksBackupGraphics();
void lagiAzelBooksRestoreGraphics(std::uint8_t* backup);

struct LagiAzelBooksSelection {
    std::array<std::uint8_t, 22> ownedIndices{};
    std::uint8_t count = 0;
    std::int32_t selected = 0;
};
// Native 06016ADA returns the table index, not the owned-list position.
std::int32_t lagiAzelBooksLookupItem(std::int32_t item);
// Native 06016328/06016396 build the owned list and choose its position.
LagiAzelBooksSelection lagiAzelBooksBuildSelection(std::int32_t requestedIndex);

struct LagiAzelBooksHeading {
    const char* title = nullptr;
    const char* heading = nullptr;
    std::uint16_t firstBodyPage = 0;
    std::uint16_t bodyPageCount = 0;
};
// Reads the loaded native MENUBK.BIN image. Returned strings borrow its
// lifetime; the caller must not retain them after releasing the resource.
bool lagiAzelBooksReadHeading(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t headingIndex, LagiAzelBooksHeading& out);
