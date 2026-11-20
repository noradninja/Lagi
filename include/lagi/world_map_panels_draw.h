#pragma once
#include "lagi/world_map_panels.h"
// Include after the Azel prelude.
namespace lagi::worldmap {
void draw_map_panels(MapPanels&,sSaturnPtr,s_workAreaCopy*,bool ordinaryMap);
}
