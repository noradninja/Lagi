#pragma once
#include <array>
#include <cstdint>
#include <vector>
struct s_workAreaCopy;
struct s_fileBundle;
struct s_3dModel;
struct sSaturnPtr;

// Named native data, not byte offsets into a polymorphic C++ task. Callback
// EAs belong to the caller's overlay and are not executable host pointers.
struct LagiBattleModelTargetState {
    std::uint16_t damageAccumulator = 0;
    std::array<std::uint32_t,4> damageCallbackEA{};
    std::array<std::int16_t,4> hitboxValues{};
    std::uint8_t auxiliaryCount = 0;
    void* auxiliaryData = nullptr;
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
