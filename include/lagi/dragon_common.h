#pragma once
namespace lagi::azel {
bool load_dragon_common_data();
bool validate_basic_wing_hotpoints(unsigned int* out_bones, unsigned int* out_hotpoints);
bool validate_basic_wing_geometry(unsigned int* out_models, unsigned int* out_vertices, unsigned int* out_polygons);
}
