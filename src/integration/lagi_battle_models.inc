// Shared BTL_T0 model/target services: 0609ED96, EE42, EE7C, EFB0, F4B0.
bool lagiCreateBattleEnemyModels(s_workAreaCopy* owner, s_fileBundle* bundle,
    std::uint8_t& modelCount, std::vector<s_3dModel>& models,
    sSaturnPtr descriptors, std::uint8_t count)
{
    modelCount = count;
    models.resize(count);
    for (unsigned i=0; i<count; ++i) {
        auto entry=descriptors+i*8;
        const auto modelOffset=readSaturnU16(entry);
        const auto poseOffset=readSaturnU16(entry+2);
        const auto hotpoints=readSaturnEA(entry+4);
        auto& model=models[i];
        const auto bones=bundle->getModelHierarchy(modelOffset)->countNumberOfBones();
        sHotpointBundle* hotpointBundle=nullptr;
        if (!hotpoints.isNull()) {
            // One deserializer owns the complete per-bone table.
            model.m_hotpointBundles.clear();
            model.m_hotpointBundles.emplace_back(hotpoints);
            hotpointBundle=model.m_hotpointBundles.data();
        }
        if (!init3DModelRawData(owner,&model,0,bundle,modelOffset,nullptr,
            bundle->getStaticPose(poseOffset,bones),nullptr,hotpointBundle)) {
            owner->getTask()->markFinished();
            return false;
        }
        stepAnimation(&model);
    }
    return true;
}

bool lagiInitBattleEnemyModelTargets(s_workAreaCopy* owner,
    std::uint32_t entityIndex, std::vector<s_3dModel>& models,
    LagiBattleModelTargetState& state,
    const std::array<std::uint32_t,4>& damageCallbackEA)
{
    // EE42: only the named native fields are initialized; don't overwrite
    // health, task headers, model vectors or adjacent actor state.
    state.damageAccumulator=0;
    state.damageCallbackEA=damageCallbackEA;
    state.hitboxValues.fill(10);
    state.auxiliaryCount=0;
    state.auxiliaryData=nullptr;

    // Host vectors replace native pointer tables. Reject malformed metadata
    // before registering ANY targets; otherwise deferred target traversal could
    // retain pointers from a failed/partially initialized actor.
    for (const auto& model:models) {
        if (!model.m40) continue;
        if (model.m40->size()<model.m12_numBones ||
            model.m44_hotpointData.size()<model.m12_numBones) {
            owner->getTask()->markFinished();return false;
        }
        std::uint32_t count=0;
        for (unsigned bone=0;bone<model.m12_numBones;++bone) {
            const auto& definition=(*model.m40)[bone];
            if (definition.m0.size()<definition.m4_count ||
                model.m44_hotpointData[bone].size()<definition.m4_count ||
                definition.m4_count>0x7fffu-count) {
                owner->getTask()->markFinished();return false;
            }
            count+=definition.m4_count;
        }
    }
    for (auto& model:models) {
        model.m50_targetBoneCount=model.m12_numBones;
        model.m52_targetableCount=0;
        model.m54_targetables=nullptr;
        model.m58_targetablePositions=nullptr;
        if (!model.m40) continue;
        for (unsigned bone=0;bone<model.m50_targetBoneCount;++bone)
            model.m52_targetableCount+=(*model.m40)[bone].m4_count;
        const auto count=model.m52_targetableCount;
        model.m54_targetables=static_cast<sBattleTargetable*>(
            allocateHeapForTask(owner,count*sizeof(sBattleTargetable)));
        model.m58_targetablePositions=static_cast<sVec3_FP*>(
            allocateHeapForTask(owner,count*sizeof(sVec3_FP)));
        unsigned index=0;
        for (unsigned bone=0;bone<model.m50_targetBoneCount;++bone) {
            const auto& definition=(*model.m40)[bone];
            for (unsigned point=0;point<definition.m4_count;++point) {
                const auto& entry=definition.m0[point];
                initTargetable(&model.m54_targetables[index],owner,
                    &model.m58_targetablePositions[index],entry.m10,entry.m0,
                    entityIndex,0,10);
                ++index;
            }
        }
    }
    return true;
}

void lagiRetireBattleEnemyModelTargets(s_workAreaCopy* owner,
    std::uint32_t& flags, std::vector<s_3dModel>& models,
    void (*beforeRetirement)(s_workAreaCopy*))
{
    flags|=0x80000000u;
    flags&=~0x40000000u;
    // Native callback is BEFORE the once-only gate, including repeat calls.
    if (beforeRetirement) beforeRetirement(owner);
    if (flags&0x10000000u) return;
    flags|=0x10000000u;
    for (auto& model:models)
        for (unsigned target=0;target<model.m52_targetableCount;++target)
            deleteTargetable(&model.m54_targetables[target]);
    // Keep arrays alive in the task heap until native task cleanup. Retiring
    // a target is not permission to free storage still referenced by shots.
}
