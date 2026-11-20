#pragma once
#include "lagi/world_map_label.h"
// Include after the Azel prelude.
namespace lagi::worldmap {
struct MapMessage {
    s_workArea* child = nullptr;
    sSaturnPtr anchor;
    MapLabelPlacement placement{0,-256,0x333}, start{};
    s32 factor = 0;
    s16 halfWidth = 0, storedTarget = 0;
    s8 mode = 3, queuedMode = -1;
};
// WORLD 060572C6 queues a new message while the current transition is active.
void request_map_message(MapMessage&, s8 mode);
// WORLD 06057708, run after the live display camera is applied.
bool update_map_message(MapMessage&, s_workArea* parent, sSaturnPtr program,
                        sSaturnPtr targets, s32 count, s32 selected, bool confirmed);
void release_map_message(MapMessage&);
}
