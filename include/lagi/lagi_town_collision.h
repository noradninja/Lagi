#pragma once

#include "lagi/lagi_town_runtime.h"

#include <array>
#include <cstdint>

namespace lagi::azel {

struct TownCollisionVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct TownCollisionSetup {
    std::int8_t collisionType = 0;
    std::int8_t isLcs = 0;
    std::int16_t collisionLayersBitField = 0;
};

// Runtime form of Azel's sCollisionBody. The owner transform remains separate
// from the registered, world-space AABB centre just as it does in Azel.
struct TownCollisionBody {
    TownCollisionSetup setup{};
    float sphereRadius = 0.0f;
    TownCollisionVec3 position{};
    TownCollisionVec3 halfAabb{};
    TownCollisionVec3 aabbCenter{};
    int setupIndex = 0;

    // Native sCollisionBody ownership/lifecycle metadata. These mirror Azel's
    // m38 owner, m3C interaction script, m40 collision model and m48 paired
    // body without moving any town-object policy into the collision solver.
    void* owner = nullptr;
    sProcessed3dModel* collisionModel = nullptr;
    TownCollisionBody* pairedBody = nullptr;
    std::uint32_t collisionScriptEA = 0;
    std::uint32_t interactionScriptEA = 0;

    TownCollisionVec3 ownerPosition{};
    TownCollisionVec3 ownerRotation{};
    std::uint32_t contactMask = 0;
    TownCollisionVec3 floorNormal{};
    TownCollisionVec3 collisionSolveTranslation{};
    unsigned contactCount = 0;
    std::uint16_t lastCollisionScriptIndex = 0;
};

void setCollisionSetup(TownCollisionBody& body, int presetIndex);
void setCollisionBounds(
    TownCollisionBody& body,
    const TownCollisionVec3& corner0,
    const TownCollisionVec3& corner1);
void resetCollisionFrame();
void registerCollisionBody(TownCollisionBody& body);
void processAllCollisions();

void beginBodyCollisionTest(TownCollisionBody& body);
void endBodyCollisionTest(TownCollisionBody& body);
void handleCollisionWithTownEnv(TownCollisionBody& body);
int getSubCellQuadrant(float worldX, float worldZ);
const TownRuntimeCell* getCellAtWorldPos(float worldX, float worldZ);
void testBodyAgainstCell(
    TownCollisionBody& body,
    const TownRuntimeCell* cell);
void processTownMeshCollision(
    TownCollisionBody& body,
    const TownRuntimeCell& cell,
    const TownRuntimeCollisionInstance& instance);
void computeCollisionSeparation(TownCollisionBody& body);

} // namespace lagi::azel
