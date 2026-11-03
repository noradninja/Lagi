#pragma once
#include <cstdint>
struct s_workArea;
namespace lagi::worldmap {
struct MapDisplayInput;
struct MapArrivalAnimation {
    std::int32_t curveFrame = 0; // native child +4, separate from model frame
    bool ready = false;
};
bool initialize_map_arrival_animation(MapArrivalAnimation&, s_workArea* owner);
void update_map_arrival_animation(MapArrivalAnimation&);
void draw_map_arrival(const MapArrivalAnimation&, MapDisplayInput&, std::int32_t cameraHeight);
}
