#pragma once
// Include after the Azel prelude.
namespace lagi::worldmap {
struct MapGround {
    std::int32_t selected=-1; // native task +24; no overlay before selection
    std::int32_t frame=0;    // +28; final frame holds rather than looping
};
// Static hierarchy/pose list in WORLD 06055924..0605596A. Optional animated
// overlays after that address are a separate native producer, not this list.
bool draw_map_ground_base(sSaturnPtr program,s_workAreaCopy* owner);
bool draw_map_ground(MapGround&,sSaturnPtr program,s_workAreaCopy* owner);
}
