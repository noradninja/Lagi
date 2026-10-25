#pragma once
#include <array>
#include <cstdint>
#include <vector>
struct s_workAreaCopy;
struct s_fileBundle;
struct s_3dModel;
struct sSaturnPtr;
struct sVec3_FP;
struct sBattleTargetable;

struct LagiBattleTargetOverride {
    std::int8_t modelIndex;
    std::int8_t targetIndex;
    std::array<std::int16_t,4> hitboxValues;
};

// Named native data, not byte offsets into a polymorphic C++ task. Callback
// EAs belong to the caller's overlay and are not executable host pointers.
struct LagiBattleModelTargetState {
    std::int16_t healthFloor = 0; // Native B4; zero disables the floor.
    std::array<std::uint32_t,4> damageCallbackEA{};
    std::array<std::int16_t,4> hitboxValues{};
    std::uint8_t auxiliaryCount = 0;
    LagiBattleTargetOverride* auxiliaryData = nullptr;
};
bool lagiCreateBattleEnemyModels(s_workAreaCopy* owner, s_fileBundle* bundle,
    std::uint8_t& modelCount, std::vector<s_3dModel>& models,
    sSaturnPtr descriptors, std::uint8_t count);
bool lagiInitBattleEnemyModelTargets(s_workAreaCopy* owner,
    std::uint32_t entityIndex, std::vector<s_3dModel>& models,
    LagiBattleModelTargetState& state,
    const std::array<std::uint32_t,4>& damageCallbackEA);
void lagiRetireBattleEnemyModelTargets(s_workAreaCopy* owner,
    std::uint32_t& flags, std::vector<s_3dModel>& models,
    void (*beforeRetirement)(s_workAreaCopy*));

struct LagiBattleModelDamageView {
    s_workAreaCopy* owner;
    std::uint32_t& flags;
    std::int16_t& health;
    std::int16_t& healthFloor;
    std::int16_t& pendingDamage;
    std::int16_t& lightColor;
    std::uint8_t& lifeMeterFlags; // sEnemyLifeMeterTask::m31 is u8.
    sVec3_FP& worldPosition;
    std::vector<s_3dModel>& models;
    LagiBattleModelTargetState& targets;
};
// Services supplied by the native actor adapter. Query quadrant/status live:
// hit callbacks may change engine/actor state during a multi-model update.
struct LagiBattleModelDamageHooks {
    void* context;
    std::uint8_t (*quadrant)(void*);
    bool (*displayWindow)(void*);
    void (*hitCallback)(void*,std::uint32_t,s_workAreaCopy*,sBattleTargetable&);
    void (*hitDisplay)(void*,s_workAreaCopy*,sBattleTargetable&,int);
    void (*damageDisplay)(void*,s_workAreaCopy*,std::int16_t,sVec3_FP*,int);
};
bool lagiUpdateBattleEnemyModelDamage(LagiBattleModelDamageView actor,
    const LagiBattleModelDamageHooks& hooks);
void lagiDrawBattleEnemyModels(std::uint32_t& flags, std::int16_t lightColor,
    const sVec3_FP& position, const sVec3_FP& rotation,
    std::vector<s_3dModel>& models);
struct LagiBattleModelMotionView {
    sVec3_FP& worldPosition;
    sVec3_FP& currentPosition;
    sVec3_FP& targetPosition;
    sVec3_FP& velocity;
    sVec3_FP& acceleration;
    sVec3_FP& worldRotation;
    sVec3_FP& currentRotation;
    sVec3_FP& targetRotation;
    sVec3_FP& angularVelocity;
    sVec3_FP& angularAcceleration;
    const sVec3_FP& parentPosition;
    const sVec3_FP& parentRotation;
    std::int32_t interpolationRate;
};
void lagiUpdateTutorialModelMotion(const LagiBattleModelMotionView& motion);
