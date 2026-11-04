#pragma once
// Include after the Azel prelude.
namespace lagi::worldmap {
// Static hierarchy/pose list in WORLD 06055924..0605596A. Optional animated
// overlays after that address are a separate native producer, not this list.
bool draw_map_ground_base(sSaturnPtr program,s_workAreaCopy* owner);
}
