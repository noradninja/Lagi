#pragma once
#include "lagi/native_battle_models.h"
struct npcFileDeleter;
struct sEnemyLifeMeterTask;
struct LagiTutorialLaserEnemyInitView {
    s_workAreaCopy* owner;
    s_fileBundle* bundle;
    std::vector<s_3dModel>& models;
    std::uint8_t& modelCount;
    LagiBattleModelTargetState& targets;
    sEnemyLifeMeterTask*& lifeMeter;
    std::int16_t& health;
    std::int32_t& entityIndex;
    LagiBattleModelMotionView motion;
    std::int32_t& interpolationRate;
};
bool lagiInitTutorialLaserEnemy(const LagiTutorialLaserEnemyInitView& actor,
    std::int32_t entityIndex, const sVec3_FP& authoredOffset,
    sSaturnPtr modelDescriptors, const std::array<std::uint32_t,4>& callbacks,
    std::uint32_t (*random)(void*), void* randomContext);
struct LagiTutorialLaserEnemyView {
    LagiBattleModelMotionView motion;
    LagiBattleModelDamageView damage;
    std::uint8_t& state;
    std::int16_t& deathTimer;
    const sVec3_FP& authoredOffset;
    std::int8_t& formationDeadCount;
    std::int32_t& experience;
    std::int32_t& dyne;
    npcFileDeleter* resources;
};
struct LagiTutorialLaserEnemyHooks {
    LagiBattleModelDamageHooks damage;
    std::uint32_t (*random)(void*);
    void (*sound)(void*,int);
    void (*recordDeath)(void*);
    void (*hideDrawing)(void*);
};
void lagiUpdateTutorialLaserEnemy(const LagiTutorialLaserEnemyView& actor,
    const LagiTutorialLaserEnemyHooks& hooks);
struct LagiTutorialLaserEnemySpawn {
    s_workAreaCopy* parent;
    npcFileDeleter* resources;
    const sVec3_FP& formationPosition;
    const sVec3_FP& formationRotation;
    std::int8_t& deadCount;
    std::int32_t& experience;
    std::int32_t& dyne;
    const sVec3_FP& authoredOffset;
    std::int32_t entityIndex;
    const sSaturnPtr& modelDescriptors;
    std::array<std::uint32_t,4> callbacks;
    LagiTutorialLaserEnemyHooks services;
};
s_workAreaCopy* lagiCreateTutorialLaserEnemyTask(const LagiTutorialLaserEnemySpawn& spawn);
struct LagiTutorialLaserFormationSpawn {
    s_workAreaCopy* parent;
    const sSaturnPtr& positions;
    const sSaturnPtr& modelDescriptors;
    std::array<std::uint32_t,4> callbacks;
    LagiTutorialLaserEnemyHooks services;
};
s_workAreaCopy* lagiCreateTutorialLaserFormation(const LagiTutorialLaserFormationSpawn& spawn);
