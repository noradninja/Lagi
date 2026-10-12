#include "lagi/azel_books_menu.h"
#include <cstdio>

int main()
{
    std::array<std::uint8_t, 512> data{};
    auto put32 = [&](unsigned offset, unsigned value) {
        for (int byte = 3; byte >= 0; --byte) { data[offset + byte] = value; value >>= 8; }
    };
    put32(0, 4);
    for (unsigned book = 0; book < 2; ++book) {
        const unsigned record = 4 + book * 16;
        put32(record, 480); put32(record + 4, 360); put32(record + 8, 400);
        data[record + 13] = 2; data[record + 15] = 4;
    }
    put32(360, 480); data[367] = 2;
    put32(368, 480); data[373] = 2; data[375] = 2;
    for (unsigned page = 0; page < 4; ++page) put32(400 + page * 4, 480);
    LagiAzelBooksSelection owned; owned.count = 2; owned.ownedIndices[1] = 1;
    LagiAzelBooksNavigation nav;
    auto move = [&](unsigned repeat) {
        return lagiAzelBooksNavigate(data.data(), data.size(), owned, nav, 0, repeat, repeat);
    };
    auto changed = move(0x80);
    if (!changed.valid || !changed.reveal || nav.indices[0] != 1 ||
        nav.indices[1] || nav.indices[2] || changed.canIncrease || !changed.canDecrease) return 10;
    changed = move(0x20 | 0x80);
    if (nav.focus != 1 || nav.indices[1] != 1 || nav.indices[2] != 2 ||
        changed.sounds != std::array<int,2>{10,10}) return 11;
    changed = move(0x20 | 0x40);
    if (nav.focus != 2 || nav.indices[2] != 1 || nav.indices[1] != 0 || !changed.reveal) return 12;
    changed = move(0x80);
    if (nav.indices[2] != 2 || nav.indices[1] != 1) return 13;
    const auto saved = nav;
    changed = lagiAzelBooksNavigate(nullptr, 0, owned, nav, 1, 0x50, 0x50);
    if (!changed.valid || !changed.cancel || changed.updateCursor ||
        nav.indices != saved.indices || nav.focus != saved.focus) return 14;
    LagiAzelBooksSelection empty;
    nav = {};
    changed = lagiAzelBooksNavigate(data.data(), data.size(), empty, nav, 0, 0x80, 0x80);
    if (!changed.valid || changed.updateCursor || changed.reveal) return 15;
    changed = lagiAzelBooksNavigate(data.data(), 16, owned, nav, 0, 0x80, 0x80);
    if (changed.valid || nav.indices != std::array<int,3>{}) return 16;
    for (int step = 0; step <= 8; ++step) {
        LagiAzelBooksWindow window{};
        if (!lagiAzelBooksCalculateWindow(0, 0x100, step, 8, window)) return 2;
        const int hw = 136 * step / 8, hh = 56 * step / 8;
        if (window.left != 176 - hw || window.right != 176 + hw ||
            window.top != 168 - hh || window.bottom != 168 + hh ||
            window.registerRight != (176 + hw) * 2 - 1 ||
            window.registerBottom != 168 + hh - 1 ||
            window.quad[0] != -4 - hw || window.quad[2] != 3 + hw) return 3;
    }
    LagiAzelBooksWindow invalid{};
    if (lagiAzelBooksCalculateWindow(0,0,1,0,invalid) ||
        lagiAzelBooksCalculateWindow(0,0,9,8,invalid)) return 4;
    unsigned cases = 0;
    for (bool vertical : {false, true}) {
        const unsigned dec = vertical ? 0x10 : 0x40;
        const unsigned inc = vertical ? 0x20 : 0x80;
        for (int maximum = 0; maximum <= 21; ++maximum)
        for (int initial = 0; initial <= maximum; ++initial)
        for (unsigned buttons = 0; buttons < 4; ++buttons)
        for (unsigned heldButtons = 0; heldButtons < 4; ++heldButtons)
        for (bool initialLatch : {false, true}) {
            int expected = initial;
            bool accepted = false;
            if (buttons & 1) { --expected; accepted = expected >= 0; }
            if (!accepted && (buttons & 2)) { ++expected; accepted = expected <= maximum; }
            if (!accepted) expected = initial;
            const bool latch = buttons ? !accepted : (heldButtons ? initialLatch : false);
            const int sound = !buttons ? -1 : (accepted ? 10 : (initialLatch ? -1 : 5));
            auto index = initial;
            bool actualLatch = initialLatch;
            const auto result = lagiAzelBooksStepIndex(index, 0, maximum,
                ((buttons & 1) ? dec : 0) | ((buttons & 2) ? inc : 0),
                ((heldButtons & 1) ? dec : 0) | ((heldButtons & 2) ? inc : 0),
                vertical, actualLatch);
            if (index != expected || result.accepted != accepted ||
                result.sound != sound || actualLatch != latch) return 1;
            ++cases;
        }
    }
    printf("PASS: %u production Books index/input cases plus coupled book/heading/body navigation, cancel and empty ownership (host, not Vita input)\n", cases);
}
