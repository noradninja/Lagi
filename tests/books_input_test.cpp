#include "lagi/azel_books_menu.h"
#include <cstdio>

int main()
{
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
    printf("PASS: %u production Books index/input cases; native priority and boundary latch (host, not Vita input)\n", cases);
}
