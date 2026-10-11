#include "lagi/azel_books_menu.h"
#include <cstdio>

int main()
{
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
