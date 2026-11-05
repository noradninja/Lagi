#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

using s32 = int;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using fixedPoint = int;
struct WorkArea { std::array<unsigned, 5> hostMetadata{}; };
using p_workArea = WorkArea*;
struct sVec3_FP {
    int m0_X = 0, m4_Y = 0, m8_Z = 0;
    int& operator[](int index) { return index == 0 ? m0_X : index == 1 ? m4_Y : m8_Z; }
};
struct sTownDragon : WorkArea {
    std::int8_t m11_subState = 0;
    sVec3_FP m58_position;
};
struct sSaturnPtr {
    const unsigned char* bytes;
    sSaturnPtr operator+(int offset) const { return {bytes + offset}; }
};
struct sEdgeTask : WorkArea {
    struct { sVec3_FP m0_position; } mE8;
    s8 mC = 0;
    sSaturnPtr m30_animationTable{};
};
struct Overlay {
    std::array<unsigned char, 0x30000> bytes{};
    sSaturnPtr getSaturnPtr(unsigned address) { return {bytes.data() + address - 0x06054000}; }
} campOverlay;
Overlay* gCurrentTownOverlay = &campOverlay;
s32 readSaturnS32(sSaturnPtr ptr) {
    return int((unsigned(ptr.bytes[0]) << 24) | (unsigned(ptr.bytes[1]) << 16) |
               (unsigned(ptr.bytes[2]) << 8) | unsigned(ptr.bytes[3]));
}
u16 readSaturnU16(sSaturnPtr ptr) { return (unsigned(ptr.bytes[0]) << 8) | ptr.bytes[1]; }
int headingX = 0, headingZ = 0;
u32 randomValue = 0;
int randomCalls = 0;
u32 randomNumber() { ++randomCalls; return randomValue; }
u32 performModulo2(u32 divisor, u32 value) { assert(divisor); return value % divisor; }
s32 atan2_FP(int x, int z) { headingX = x; headingZ = z; return 123; }
struct Animation { int source; unsigned offset; };
using sAnimationData = Animation;
struct Bundle {
    int source;
    Animation animation{};
    Animation* getAnimation(unsigned offset) { animation = {source, offset}; return &animation; }
} npcBundle{1}, globalBundle{2};
struct FileBundle { Bundle* m0_fileBundle; } globalFileBundle{&globalBundle};
struct { FileBundle* mC_fileBundle; } dramAllocatorEnd[] = {{&globalFileBundle}};
struct Model { Animation* animation = nullptr; int interpolation = 0; };
void playAnimationGeneric(Model* model, Animation* animation, int interpolation) {
    model->animation = animation;
    model->interpolation = interpolation;
}
struct sNPC {
    unsigned sentinel = 0x12345678;
    s8 mC = 0, mF = 0, mE_controlState = 0;
    int m2C_currentAnimation = 0, m148_savedAngle = 0;
    int m178 = 0, m179 = 0, m17A = 0;
    std::array<s8, 32> m158_animQueue{};
    Bundle* m0_fileBundle = &npcBundle;
    sSaturnPtr m30_animationTable{};
    Model m34_3dModel;
    std::array<int, 2> m20_lookAtAngle{};
    struct { sVec3_FP m0_position, mC_rotation, m48_targetRotation; } mE8;
};
struct Proxy { p_workArea workArea = nullptr; sNPC* pNPC = nullptr; };
struct {
    std::array<Proxy, 16> m70_npcPointerArray{};
    struct { p_workArea m8_owner = nullptr; } m104_currentScript;
} npcData0;
struct {
    std::array<std::uint8_t, 0x264> bitField{};
    struct {
        short m10_currentHP = 0, m14_currentBP = 0;
        short mB8_maxHP = 100, mBA_maxBP = 50;
    } gameStats;
} mainGameState;
int resourceCalls = 0, resourceType = -1, resourceAmount = -1;
void increaseGameResource(int type, int amount) {
    ++resourceCalls;
    resourceType = type;
    resourceAmount = amount;
}
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
        assert(lagiCampFeedingFinished(9) == (substate < 12));
        mainGameState.bitField[0x263] = 0x80;
        assert(lagiCampFinishInteraction(9) == -128);
        assert(dragons[9].m11_subState == (substate < 9 ? 7 : substate));
    }
    assert(lagiCampStartFeeding(9) == 0x11);
    assert(dragons[9].m11_subState == 12);
    for (unsigned flags = 0; flags < 256; ++flags) {
        npcData0.m70_npcPointerArray[1].pNPC = &unrelated;
        unrelated.mC = static_cast<s8>(flags);
        assert(lagiCampNpcMotionFinished(1) == ((flags & 6) == 0));
        assert(lagiCampDisableLookAtDecay(1) == (int(s8(flags)) | 8));
        assert(std::uint8_t(unrelated.mC) == (flags | 8));
        assert(lagiCampEnableLookAtDecay(1) == int(flags & 0xF7));
        assert(std::uint8_t(unrelated.mC) == (flags & 0xF7));
        assert(unrelated.sentinel == 0x12345678);
        edge.mC = static_cast<s8>(flags);
        assert(lagiCampUseScriptedPlayerAnimations() == (int(s8(flags)) | 1));
        assert(std::uint8_t(edge.mC) == (flags | 1));
        assert(edge.m30_animationTable.bytes == campOverlay.getSaturnPtr(0x0607FA84).bytes);
        assert(lagiCampUseNormalPlayerAnimations() == int(flags & 0xFE));
        assert(std::uint8_t(edge.mC) == (flags & 0xFE));
        assert(edge.m30_animationTable.bytes == campOverlay.getSaturnPtr(0x0608015C).bytes);
        mainGameState.bitField.fill(0x55);
        mainGameState.bitField[0x263] = flags;
        resourceCalls = 0;
        int result = lagiCampRewardPetting();
        assert(result == (flags & 0x80 ? -128 : int(s8(flags | 0x80))));
        assert(resourceCalls == (flags & 0x80 ? 0 : 1));
        if (resourceCalls) assert(resourceType == 0xB && resourceAmount == 2);
        assert(mainGameState.bitField[0x263] == (flags | 0x80));
        assert(mainGameState.bitField[0x262] == 0x55);
        resourceCalls = 0;
        assert(lagiCampRewardPetting() == -128 && resourceCalls == 0);
        mainGameState.bitField[0x262] = flags;
        result = lagiCampRewardInteraction();
        assert(result == (flags & 1 ? 1 : int(s8(flags | 1))));
        assert(resourceCalls == (flags & 1 ? 0 : 1));
        if (resourceCalls) assert(resourceType == 0xB && resourceAmount == 1);
        assert(mainGameState.bitField[0x262] == (flags | 1));
        assert(mainGameState.bitField[0x263] == (flags | 0x80));
        resourceCalls = 0;
        assert(lagiCampRewardInteraction() == 1 && resourceCalls == 0);
    }
    std::puts("PASS native feeding transitions, signed completion thresholds and once-only affinity rewards");
    std::puts("PASS production town save request uses the current status and native zero entry indices");
    sNPC actor, target;
    npcData0.m70_npcPointerArray[1].pNPC = &actor;
    npcData0.m70_npcPointerArray[2].pNPC = &target;
    std::array<unsigned char, 64> animations{};
    animations[3 * 4 + 3] = 12;
    animations[5 * 4 + 3] = 20;
    actor.m30_animationTable = {animations.data()};
    actor.m178 = actor.m179 = actor.m17A = 7;
    assert(lagiCampSetNpcAnimation(1, 0, 3) == 2);
    assert(actor.m178 == 0 && actor.m179 == 0 && actor.m17A == 0);
    assert(actor.mE_controlState == 2 && actor.m2C_currentAnimation == 0);
    assert(actor.m34_3dModel.animation == nullptr && actor.m34_3dModel.interpolation == 10);
    assert(lagiCampSetNpcAnimation(1, 5, 3) == 3);
    assert(actor.mE_controlState == 3 && actor.m2C_currentAnimation == 5);
    assert(actor.m34_3dModel.animation->source == 1 && actor.m34_3dModel.animation->offset == 20);
    animations[5 * 4 + 1] = 1;
    lagiCampSetNpcAnimation(1, 5, 1);
    assert(actor.m34_3dModel.animation->source == 2);
    assert(lagiCampSetNpcAnimation(1, 261, 259) == 3);
    assert(actor.m2C_currentAnimation == 5 && actor.mE_controlState == 3);
    actor.mE8.m0_position = {120, 0, -300};
    target.mE8.m0_position = {20, 0, -100};
    actor.mE8.mC_rotation.m4_Y = 456;
    actor.m20_lookAtAngle[1] = 222;
    actor.mF = 4;
    lagiCampTurnNpcTowardsNpc(1, 2, 0);
    assert(headingX == 100 && headingZ == -200);
    assert(actor.mE8.m48_targetRotation.m4_Y == 123 && actor.m148_savedAngle == 456);
    assert(actor.mF == 2 && (actor.mC & 4) && actor.m2C_currentAnimation == 3);
    lagiCampTurnNpcTowardsNpc(1, 2, 1);
    assert(actor.mE8.m48_targetRotation.m4_Y == -333 && actor.m148_savedAngle == 222);
    assert(actor.mF == 6);
    lagiCampTurnNpcBack(1, 0);
    assert(actor.mE8.m48_targetRotation.m4_Y == 222 && actor.m148_savedAngle == 456);
    lagiCampTurnNpcBack(1, 1);
    assert(actor.mE8.m48_targetRotation.m4_Y == 456 && actor.m148_savedAngle == 222);
    assert(lagiCampScheduleNpcAnimation(1, 261, 259) == 3);
    assert(actor.m178 == 1 && actor.m179 == 0 && actor.m17A == 1);
    assert(actor.m158_animQueue[0] == 5 && actor.m158_animQueue[1] == 3);
    assert(actor.mE_controlState == 2);
    actor.mE_controlState = 0;
    assert(lagiCampScheduleNpcAnimation(1, 256, 3) == 0x159);
    assert(actor.m158_animQueue[1] == 0 && actor.mE_controlState == 0);
    std::puts("PASS reused NPC turning, saved-angle restoration and animation/queue callbacks");
    for (u32 value : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
        for (u32 limit : {1u, 2u, 3u, 16u, 0xffffffffu}) {
            randomValue = value;
            randomCalls = 0;
            assert(u32(lagiCampRandomChoice(s32(limit))) == value % limit);
            assert(randomCalls == 1);
        }
    }
}
