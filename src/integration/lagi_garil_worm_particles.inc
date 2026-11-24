// Shared by the environmental and story worms; original 0605F86C/0605F804.
void lagiSpawnA5WormSegmentSand(sVec3_FP* position) {
    auto* manager=static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC)->m54_particlePool;
    const auto* quads=a5GetOrParseQuadList(gFLD_A5->getSaturnPtr(0x0608C6E4));
    for(int i=0;i<4;++i) {
        sVec3_FP p=*position,velocity={0,0,0};
        p.m0_X+=centeredRandom(0xFFFF);p.m8_Z+=centeredRandom(0xFFFF);
        sParticleSpawnConfig desc={};desc.m8_pQuadData=quads;
        spawnParticleProjected(manager,&desc,&p,&velocity);
    }
}
void lagiSpawnA5WormSegmentDebris(const std::shared_ptr<sMonsterBody>& body,int part) {
    auto* manager=static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC)->m54_particlePool;
    sVec3_FP velocity={0,0,0};sParticleSpawnConfig desc={};
    desc.m4_pVelocity=&velocity;
    desc.m8_pQuadData=a5GetOrParseQuadList(gFLD_A5->getSaturnPtr(0x0608C5E8));
    // Aliasing ownership keeps the segment position valid after actor teardown.
    const std::shared_ptr<sVec3_FP> position(body,&body->m30_parts[part].m4_worldPosition);
    lagiSpawnFieldParticleFollowingXZ(manager,desc,position,2);
}
