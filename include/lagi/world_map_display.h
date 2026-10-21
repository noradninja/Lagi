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
// Recover native Update without invoking the still-unported WORLD task graph.
// False leaves state unchanged when a selected native table entry is invalid.
bool update_map_display(MapDisplay&,const MapDisplayInput&);
void apply_map_camera_and_light(const MapDisplay&);
bool draw_map_trail(const MapDisplay&,s_workAreaCopy*);
}
