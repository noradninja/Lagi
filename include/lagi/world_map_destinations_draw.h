#pragma once
// Include after the Azel prelude. Native WORLD child0605637C owns this state.
namespace lagi::worldmap {
struct MapDestinationModels;
MapDestinationModels* initialize_map_destination_models(sSaturnPtr,s_workAreaCopy*);
bool draw_map_destination_models(MapDestinationModels*,sSaturnPtr,sSaturnPtr,s32,s32,s_workAreaCopy*);
void delete_map_destination_models(MapDestinationModels*);
}
