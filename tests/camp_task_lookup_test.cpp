#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

using s32 = int;
struct WorkArea { std::array<unsigned, 5> hostMetadata{}; };
using p_workArea = WorkArea*;
struct sVec3_FP { int m0_X = 0, m4_Y = 0, m8_Z = 0; };
struct sTownDragon : WorkArea {
    std::int8_t m11_subState = 0;
    sVec3_FP m58_position;
};
struct sEdgeTask : WorkArea { struct { sVec3_FP m0_position; } mE8; };
struct sSaturnPtr {
    const unsigned char* bytes;
    sSaturnPtr operator+(int offset) const { return {bytes + offset}; }
};
s32 readSaturnS32(sSaturnPtr ptr) {
    return int((unsigned(ptr.bytes[0]) << 24) | (unsigned(ptr.bytes[1]) << 16) |
               (unsigned(ptr.bytes[2]) << 8) | unsigned(ptr.bytes[3]));
}
int headingX = 0, headingZ = 0;
s32 atan2_FP(int x, int z) { headingX = x; headingZ = z; return 123; }
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
struct { unsigned m4_gameStatus = 0; } gGameStatus;
struct { unsigned m8_gameMode = 0, m9_fieldIndex = 0, mA_subFieldIndex = 0; int mB_entryPointIndex = 0; } gSaveGameStatus;
int requestedMenu = 0;
void setupSaveParams(int field, int subfield, int entry) {
    requestedMenu = 8;
    // Deliberately differ from the live town status to exercise the native
    // status source, including entry after a movie with an older saved route.
    gSaveGameStatus = {0x54, unsigned(field), unsigned(subfield), entry};
}

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
    sEdgeTask edge;
    edge.mE8.m0_position = {120, 456, -300};
    npcData0.m70_npcPointerArray[0].workArea = &edge;
    for (int index = 1; index < 16; ++index) {
        std::array<unsigned char, 0x24> record{};
        record[0x23] = static_cast<unsigned char>(index);
        npcData0.m70_npcPointerArray[index].workArea = nullptr;
        registerProductionDragon(&dragons[index], {record.data()});
        assert(npcData0.m70_npcPointerArray[0].workArea == &edge);
        assert(getNpcDataByIndexAsTask(index) == &dragons[index]);
        mainGameState.gameStats.m10_currentHP = 1;
        assert(scriptFunction_6056484(index) == 1);
        dragons[index].m58_position = {20, 99, -100};
        assert(sCampDragon_InitSub1(&dragons[index]) == 123);
        assert(headingX == 100 && headingZ == -200);
    }
    std::puts("PASS production Camp task lookup and HP/BP recovery across all slots and script owner");
    std::puts("PASS production dragon registration preserves Edge and typed heading consumes its position");
    for (unsigned status : {0x52u, 0x55u}) {
        gGameStatus.m4_gameStatus = status;
        gSaveGameStatus = {1, 2, 3, 4};
        requestedMenu = 0;
        assert(lagiTownOpenSaveMenu() == 0);
        assert(requestedMenu == 8);
        assert(gSaveGameStatus.m8_gameMode == status);
        assert(gSaveGameStatus.m9_fieldIndex == 0);
        assert(gSaveGameStatus.mA_subFieldIndex == 0);
        assert(gSaveGameStatus.mB_entryPointIndex == 0);
    }
    for (int substate = -128; substate <= 127; ++substate) {
        dragons[9].m11_subState = static_cast<std::int8_t>(substate);
        assert(lagiCampRecoveryFinished(9) == (substate < 10));
    }
    std::puts("PASS production town save request uses the current status and native zero entry indices");
}
