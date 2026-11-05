#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

using s32 = int;
struct WorkArea { std::array<unsigned, 5> hostMetadata{}; };
using p_workArea = WorkArea*;
struct sTownDragon : WorkArea { std::int8_t m11_subState = 0; };
struct sNPC { unsigned sentinel = 0x12345678; };
struct Proxy { p_workArea workArea = nullptr; sNPC* pNPC = nullptr; };
struct {
    std::array<Proxy, 16> m70_npcPointerArray{};
    struct { p_workArea m8_owner = nullptr; } m104_currentScript;
} npcData0;
struct {
    struct {
        short m10_currentHP = 0, m14_currentBP = 0;
        short mB8_maxHP = 100, mBA_maxBP = 50;
    } gameStats;
} mainGameState;

#include "production.inc"

int main() {
    std::array<sTownDragon, 16> dragons;
    sNPC unrelated;
    for (int index = 0; index < 16; ++index) {
        auto& dragon = dragons[index];
        dragon.hostMetadata.fill(0xaabbccdd);
        npcData0.m70_npcPointerArray[index].workArea = &dragon;
        // The crash had a registered task and an empty sNPC slot. Also check
        // that a distinct, non-null sNPC slot cannot redirect the operation.
        for (sNPC* npc : {static_cast<sNPC*>(nullptr), &unrelated}) {
            npcData0.m70_npcPointerArray[index].pNPC = npc;
            for (int healthState = 0; healthState < 4; ++healthState) {
                mainGameState.gameStats.m10_currentHP = healthState & 1 ? 99 : 100;
                mainGameState.gameStats.m14_currentBP = healthState & 2 ? 49 : 50;
                dragon.m11_subState = 3;
                assert(getNpcDataByIndexAsTask(index) == &dragon);
                assert(scriptFunction_6056484(index) == (healthState != 0));
                assert(dragon.m11_subState == (healthState ? 10 : 3));
                assert(mainGameState.gameStats.m10_currentHP == 100);
                assert(mainGameState.gameStats.m14_currentBP == 50);
                for (unsigned word : dragon.hostMetadata) assert(word == 0xaabbccdd);
                assert(unrelated.sentinel == 0x12345678);
            }
        }
    }
    npcData0.m104_currentScript.m8_owner = &dragons[7];
    assert(getNpcDataByIndexAsTask(-1) == &dragons[7]);
    mainGameState.gameStats.m10_currentHP = 1;
    assert(scriptFunction_6056484(-1) == 1);
    assert(dragons[7].m11_subState == 10);
    std::puts("PASS production Camp task lookup and HP/BP recovery across all slots and script owner");
}
