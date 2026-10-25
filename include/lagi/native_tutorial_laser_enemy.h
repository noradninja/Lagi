#pragma once
#include "lagi/native_battle_models.h"
struct npcFileDeleter;
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
