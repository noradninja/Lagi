#pragma once
#include <cstdint>
struct s_workArea;
struct s_workAreaCopy;
struct sVec3_FP;
struct sSaturnPtr;

// Authored homing projectiles share rendering, resources and dragon damage.
// Quad and damage parameters belong to the requesting original overlay.
s_workArea* lagiCreateNativeHomingBattleProjectile(s_workAreaCopy* parent,
    const sVec3_FP& source,const sVec3_FP& velocity,sSaturnPtr quad,
    std::uint8_t slot,std::int16_t damage,std::uint8_t damageType);

// Parent owns the live world-space hotpoint and state for this child task.
// Charge quads have private mutable frame dimensions, never shared-cache writes.
s_workArea* lagiCreateNativeBattleChargeQuad(s_workAreaCopy* parent,
    const sVec3_FP* position,const std::uint8_t* state,std::uint8_t activeState,
    sSaturnPtr quad,std::uint8_t slot);

// Shared native sphere contact: transform both world positions into battle
// view space and return the original midpoint transformed back to world space.
// Callers retain their authored radii and damage-position/attack-kind policy.
bool lagiBattleContactPoint(const sVec3_FP& source,std::int32_t sourceRadius,
    const sVec3_FP& target,std::int32_t targetRadius,sVec3_FP& worldContact);
