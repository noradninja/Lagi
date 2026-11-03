#pragma once
#include "lagi/world_map_model.h"

namespace lagi::worldmap {
struct MapTrailPoint {sVec3_FP position;fixedPoint yaw;};
struct MapDisplay {
    MapModel marker;
    sVec3_FP markerVelocity;               // native +68
    s32 yawVelocity=0;                     // +78
    s32 poseCounter=0;                     // +80 (low half is +82)
    sVec3_FP cameraVelocity;               // +88
    sVec3_FP desiredCamera,desiredTarget;  // +94 / +A0
    sVec3_FP camera,target,rotation;       // +AC / +B8 / +C4
    s32 orbitAngle=0,orbitVelocity=0;      // +D0 / +D4
    std::array<MapTrailPoint,16> trail;    // +D8
    s32 trailHead=0,trailCount=0,trailTick=0;
    s32 state=0,previousState=0,selectedTarget=0; // +1E4 / +1E8 / +1EC
    s32 lightYaw=0,colorBlend=0;           // +1F4 / +208
    s8 orbitControl=0;                    // +20C
};
struct MapDisplayInput {
    sSaturnPtr program; // WORLD at native base 06054000
    sSaturnPtr targetTable;
    sVec3_FP arrivalPosition; // native root +14 child's live +8/+C/+10 position
    bool arrivalReady=false;
    s32 targetCount=0;
    s32 cameraSelection=0;
    bool arrivalBusy=false; // root +14 child's +3D; do not infer title save UI
};
// Camera Init 06057234 selects the ordinary or fleet destination table.
// savedRoute is the six-bit value at packed game-state index 0x87.
inline bool initialize_map_destinations(MapDisplayInput& input, sSaturnPtr program,
                                       bool fleet, unsigned savedRoute)
{
    if (program.isNull() || savedRoute >= 64 || (!fleet && savedRoute >= 33))
        return false;

    input.program = program;
    input.targetTable = program + (fleet ? 0x5B2C : 0x5688);
    input.targetCount = fleet ? 4 : 33;
    // Ordinary WORLD restores this index when the environment becomes ready.
    input.cameraSelection = fleet ? 0 : static_cast<s32>(savedRoute);
    if (fleet) {
        switch (savedRoute + 0x50) {
        case 0x56: input.cameraSelection = 3; break;
        case 0x67: input.cameraSelection = 2; break;
        case 0x73: input.cameraSelection = 1; break;
        }
    }
    return true;
}
struct MapDestination {
    s16 storyBit;
    s16 gameStatus;
    s8 preloadIndex; // WORLD camera passes +4 to native field/town preload helpers
    s8 type;

    bool canTravel() const { return gameStatus >= 0 && type >= 0 && type <= 3; }
};

inline MapDestination read_map_destination(sSaturnPtr entry)
{
    return {readSaturnS16(entry), readSaturnS16(entry + 2),
            readSaturnS8(entry + 4), readSaturnS8(entry + 5)};
}
// Camera state 7: MAIN 06011E0A (field) / 06030100 (town).
// Preparation does not activate an overlay or change field/story state.
bool prepare_map_travel(const MapDestination&);
// WORLD arrival Init 0605532A positions the child above its authored entry.
bool initialize_map_arrival(MapDisplayInput&, s32 selected);
bool retarget_map_arrival(MapDisplayInput&, s32 selected, fixedPoint& yaw,
                          s32& distance, s32& targetHeight);
void update_map_arrival_motion(MapDisplayInput&, fixedPoint yaw,s32 distance,s32 targetHeight,
                               s32& forwardStep, s32& verticalStep);
// Recover native Update without invoking the still-unported WORLD task graph.
// False leaves state unchanged when a selected native table entry is invalid.
bool update_map_display(MapDisplay&,const MapDisplayInput&);
s32 find_map_destination(const MapDisplayInput&, s32 current, unsigned direction);
void apply_map_camera_and_light(const MapDisplay&);
bool draw_map_trail(const MapDisplay&,s_workAreaCopy*);
}
