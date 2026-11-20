#pragma once
// Include after the Azel prelude.
#include "lagi/world_map_label.h"
namespace lagi::worldmap {
// MAIN 060263D4 / 06026316. The parent owns the task and keeps placement alive.
// WORLD's authored maximum is 40 cells in reserved VDP1 range 7B600..7BFFF.
s_workArea* create_map_sprite_text(s_workArea* parent, sSaturnPtr string,
                                  const MapLabelPlacement* placement);
}
