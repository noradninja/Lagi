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

struct LagiAzelBooksBodyPage {
    // Native values 0..3 are presentation commands, not string offsets.
    std::uint32_t nativeValue = 0;
    const char* text = nullptr;
};
bool lagiAzelBooksReadBodyPage(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t bodyPageIndex, LagiAzelBooksBodyPage& out);
// Native title, heading, page counter and body text regions. Illustration
// commands are returned to the owning task, not drawn as invented text.
bool lagiAzelBooksPresentText(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t headingIndex,
    std::uint32_t bodyPageIndex, LagiAzelBooksBodyPage& body);
// Native 060166C2 updates the heading after a body-page turn; unmatched
// pages select heading zero, as in the original routine.
bool lagiAzelBooksHeadingForBodyPage(const std::uint8_t* data, std::size_t bytes,
    std::uint32_t bookIndex, std::uint32_t bodyPageIndex, std::uint32_t& headingIndex);

struct LagiAzelBooksIndexStep {
    bool accepted = false;
    int sound = -1;
};

struct LagiAzelBooksWindow {
    std::int32_t left, top, right, bottom;
    // Native register X is doubled; ending coordinates are inclusive.
    std::int32_t registerLeft, registerTop, registerRight, registerBottom;
    std::array<std::int32_t, 8> quad;
};
// Native 06016088 window/quad calculation; positive denominator required.
bool lagiAzelBooksCalculateWindow(std::int32_t scrollX, std::int32_t scrollY,
    std::int32_t step, std::int32_t duration, LagiAzelBooksWindow& out);
// Native register writes and VDP1 polygon submission for that window.
bool lagiAzelBooksPresentWindow(std::int32_t scrollX, std::int32_t scrollY,
    std::int32_t step, std::int32_t duration);
// Native 06015D30 (left/right) and 06015DA4 (up/down). The caller supplies
// the native repeat and held masks and retains a separate edge latch per axis.
LagiAzelBooksIndexStep lagiAzelBooksStepIndex(std::int32_t& index,
    std::int32_t minimum, std::int32_t maximum, std::uint16_t repeated,
    std::uint16_t held, bool vertical, bool& boundarySoundLatched);

struct LagiAzelBooksNavigation {
    std::int32_t focus = 0;
    std::array<std::int32_t, 3> indices{}; // owned-list position, heading, body
    bool verticalBoundary = false;
    bool horizontalBoundary = false;
};
struct LagiAzelBooksNavigationResult {
    bool valid = false;
    bool cancel = false;
    bool reveal = false;
    bool updateCursor = false;
    bool canDecrease = false;
    bool canIncrease = false;
    std::array<int, 2> sounds{-1, -1};
};
// Original 06016738. Resource and ownership are borrowed for this call.
LagiAzelBooksNavigationResult lagiAzelBooksNavigate(
    const std::uint8_t* data, std::size_t bytes,
    const LagiAzelBooksSelection& owned, LagiAzelBooksNavigation& navigation,
    std::uint16_t pressed, std::uint16_t repeated, std::uint16_t held);
