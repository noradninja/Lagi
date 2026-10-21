#pragma once

// Included after the Azel prelude. These are typed host fields, not a packed
// Saturn task overlay: native pointers cannot be cast into a Vita task object.
namespace lagi::worldmap {
struct MapModel {
    s_3dModel model;                    // WORLD display +0x0C
    sVec3_FP position;                  // +0x5C
    fixedPoint yaw = 0;                 // +0x74
    fixedPoint scale = 0;               // +0x7C
    s16 poseControl = 0;                // +0x82
    s16 transitionRate = 0x2000;        // +0x84
    s32 lightAngle = 0;
    std::array<u32, 4> lightColors{};   // +0x1F8..0x204
    u8 flags = 0;                      // +0x20D
};

// Native WORLD 06054504 and 060551AC, separated from the unfinished camera,
// environment and save task graph. Not a replacement map or live entry point.
bool initialize_map_model(MapModel&, s_workAreaCopy*, sSaturnPtr lightTable);
void draw_map_model(MapModel&);
}
