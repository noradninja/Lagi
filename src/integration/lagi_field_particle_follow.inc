// Original A5 060805C8: follow live X/Z, keep Y at the supplied offset,
// and retire after the authored number of animated-quad cycle completions.
struct LagiFieldParticleFollowPayload {
    std::shared_ptr<sVec3_FP> position;
    int cycles;
};
static s32 lagiUpdateFieldParticleFollowingXZ(sParticleSlot* slot) {
    auto& payload=*static_cast<LagiFieldParticleFollowPayload*>(slot->m20_heapData);
    const auto& p=*payload.position;
    slot->m0_position.m0_X=p.m0_X+slot->mC_velocity.m0_X;
    slot->m0_position.m4_Y=slot->mC_velocity.m4_Y;
    slot->m0_position.m8_Z=p.m8_Z+slot->mC_velocity.m8_Z;
    if((sGunShotTask_UpdateSub4(&slot->m2C_animQuad)&2)&&--payload.cycles<=0)return 1;
    return 0;
}
static void lagiReleaseFieldParticlePayload(sParticleSlot* slot) {
    if(slot->m20_heapData&&slot->m24_updateFunc==lagiUpdateFieldParticleFollowingXZ)
        static_cast<LagiFieldParticleFollowPayload*>(slot->m20_heapData)->~LagiFieldParticleFollowPayload();
}
static void lagiDeleteFieldParticlePayloads(sParticlePoolManager* pool) {
    for(int i=0;i<pool->m14_maxParticles;++i) {
        auto& slot=pool->m8_slotsBase[i];
        if(slot.m20_heapData) {
            lagiReleaseFieldParticlePayload(&slot);
            freeHeapForTask(pool,slot.m20_heapData);slot.m20_heapData=nullptr;
        }
        slot.m28_drawFunc=nullptr;
    }
    pool->m18_activeCount=0;
}
int lagiSpawnFieldParticleFollowingXZ(sParticlePoolManager* pool,const sParticleSpawnConfig& source,
                                     const std::shared_ptr<sVec3_FP>& position,int animationCycles) {
    // spawnParticleInPool copies POD bytes. Construct the owning C++ payload
    // in its allocated storage only after the slot has been successfully published.
    unsigned char empty[sizeof(LagiFieldParticleFollowPayload)]={};
    auto desc=source;
    desc.m0_pPosition=position.get();desc.m14_updateFunc=lagiUpdateFieldParticleFollowingXZ;
    desc.m18_heapSize=sizeof(empty);desc.m1C_heapData=empty;
    if(!spawnParticleInPool(pool,&desc,0))return 0;
    new(pool->mC_currentSlot->m20_heapData) LagiFieldParticleFollowPayload{position,animationCycles};
    return 1;
}
