// Original BTL_A5_3 060541AC and 060542B0. Included by the generated
// overlay adapter; all storage is typed host storage, not Saturn byte offsets.
void initBattleTargetables(p_workArea, s_3dModel*, s16*, std::vector<sBattleTargetable>&, std::vector<sVec3_FP>&);

struct LagiLathumRig {
    struct Group {
        sVec3_FP position;
        std::vector<sBattleTargetable> targets;
        u8 state = 0;
        u8 targetCount = 0;
        u8 timer = 0;
        u8 phase = 0;
    };
    std::vector<s_3dModel> models;
    u8 modelCount = 0;
    s_3dModel emergenceModel;
    s16 coreTargetCount = 0;
    std::vector<sBattleTargetable> coreTargets;
    std::vector<sVec3_FP> corePositions;
    sVec3_FP position;
    sVec3_FP localPosition;
    sEnemyLifeMeterTask* lifeMeter = nullptr;
    s16 health = 0;
    s16 pendingDamage = 0; // Native 0E.
    u8 damagePending = 0; // Native 12.
    u8 selectedHitCount = 0; // Native 188.
    u8 substate = 0; // Native 09.
    s16 controllerTimer = 0; // Native FC.
    sVec3_FP cameraEye, cameraTarget; // Native 114/120.
    p_workArea cameraTransition = nullptr; // Native 130.
    p_workArea dialogue = nullptr; // Native 12C.
    p_workArea ring = nullptr; // Native 134.
    u8 activeRingCount = 0; // Native 189.
    sVec3_FP dragonDisplacement; // Native 104/108/10C.
    std::array<Group,9> groups;
    u8 selectedGroup = 0;
    s8 groupCounter = -1;
    u8 mode = 4;
    u8 modelPhase = 0;
    u8 hideBody = 0; // Native 18A; consumed by Draw outside phase 0.
    sSaturnPtr descriptor;
};

static bool lagiInitializeLathumRig(s_workAreaCopy* owner, LagiLathumRig& rig) {
    auto* file=allocateNPC(owner,8);
    if (!file || !file->m0_fileBundle) return false;
    // Preserve the native resource identity for children/model submissions.
    owner->m0_fileBundle=file->m0_fileBundle;
    owner->m4_vd1Allocation=file->m4_vd1Allocation;
    rig.descriptor=g_BTL_A5_3->getSaturnPtr(0x060A3EB4);
    rig.localPosition=readSaturnVec3(g_BTL_A5_3->getSaturnPtr(0x060A3EC8));
    if (!lagiCreateBattleEnemyModels(owner,file->m0_fileBundle,rig.modelCount,
        rig.models,readSaturnEA(rig.descriptor+8),1)) return false;
    auto& model=rig.models[0];
    // This service uses the original null-owner core-target contract. Lathum
    // consumes damage itself; its nine extra groups have independent flags.
    initBattleTargetables(owner,&model,&rig.coreTargetCount,rig.coreTargets,rig.corePositions);
    rig.lifeMeter=createEnemyLifeMeterTask(&rig.position,0,&rig.health,
        readSaturnU8(rig.descriptor+1));
    const auto animationTable=readSaturnEA(rig.descriptor+0xC);
    initAnimation(&model,file->m0_fileBundle->getAnimation(readSaturnU16(animationTable)));

    rig.selectedGroup=performModulo2(9,randomNumber());
    rig.groupCounter=-1;
    for (unsigned group=0;group<rig.groups.size();++group) {
        auto& value=rig.groups[group];
        value.targetCount=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3EFB+group));
        value.targets.resize(value.targetCount);
        for (unsigned point=0;point<value.targets.size();++point)
            initTargetable(&value.targets[point],nullptr,&value.position,0xA000,
                point?0xF0004002u:0xF0004000u,0,0,10);
    }
    displayFormationName(0,1,13);
    rig.mode=4; rig.modelPhase=0;
    const auto bones=file->m0_fileBundle->getModelHierarchy(0xC)->countNumberOfBones();
    return init3DModelRawData(owner,&rig.emergenceModel,0,file->m0_fileBundle,0xC,
        file->m0_fileBundle->getAnimation(0x158),
        file->m0_fileBundle->getStaticPose(0x14C,bones),nullptr,nullptr)!=0;
}
