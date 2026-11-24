// BTL_A5_3 06056E2E / 06056A60. This child supplies the seeking ring
// visuals; mode2 owns displacement and damage. No collision/damage substitute.
struct LagiLathumRing : s_workAreaTemplate<LagiLathumRing> {
    sVec3_FP position,velocity;
    u32 angle=0;
    s16 remaining=0;
    npcFileDeleter* file=nullptr;
    static void Update(LagiLathumRing* self) {
        const auto& dragonPosition=gBattleManager->m10_battleOverlay->m18_dragon->m8_position;
        self->velocity[0]=MTH_Mul(0xCCC,self->velocity[0]);
        self->velocity[2]=MTH_Mul(0xCCC,self->velocity[2]);
        for(unsigned axis:{0u,2u}) {
            const auto delta=lagi::formationSubtract(dragonPosition[axis].asS32(),self->position[axis].asS32());
            self->velocity[axis]=lagi::formationAdd(self->velocity[axis].asS32(),MTH_Mul(0xCCC,fixedPoint(delta)).asS32());
        }
        for(unsigned axis=0;axis<3;++axis)
            self->position[axis]=lagi::formationAdd(self->position[axis].asS32(),self->velocity[axis].asS32());
        self->angle+=0x71C71C;
        for(unsigned quadrant=0;quadrant<4;++quadrant) {
            if(!(randomNumber()&1))continue;
            const auto phase=(self->angle+quadrant*0x04000000u)>>16;
            sVec3_FP particle=self->position,zero;
            // Native COMMON table20216E80 is sine,20217E80 is cosine.
            particle[0]=lagi::formationAdd(particle[0].asS32(),MTH_Mul(0xA000,getSin(phase)).asS32());
            particle[2]=lagi::formationAdd(particle[2].asS32(),MTH_Mul(0xA000,getCos(phase)).asS32());
            createParticleEffect(self->file,lagiNativeBattleParticleQuad(g_BTL_A5_3->getSaturnPtr(0x060A3AD8)),&particle,&zero,nullptr,0x30000,nullptr,15);
        }
        self->remaining=lagiLathumDamageWord(s32(self->remaining)-1);
        if(self->remaining>=0)return;
        // Native06056E0E explicitly clears the dragon status word on expiry.
        gBattleManager->m10_battleOverlay->m18_dragon->m1C0_statusModifiers=0;
        self->getTask()->markFinished();
    }
    static TypedTaskDefinition* getTypedTaskDefinition() {
        static TypedTaskDefinition definition={nullptr,&Update,nullptr,nullptr};return &definition;
    }
};
static p_workArea lagiCreateLathumRing(s_workAreaCopy* owner,LagiLathumRig& rig,
    u8 group,const sVec3_FP& dragonPosition) {
    auto* ring=createSubTask<LagiLathumRing>(owner);if(!ring)return nullptr;
    const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+group));
    transformAndAddVec(rig.corePositions[anchor],ring->position,cameraProperties2.m28[1]);
    rig.cameraTarget=ring->position;
    for(unsigned axis=0;axis<3;++axis) {
        const auto delta=lagi::formationSubtract(dragonPosition[axis].asS32(),ring->position[axis].asS32());
        ring->velocity[axis]=MTH_Mul(FP_Div(0x10000,0x1E0000),fixedPoint(delta));
    }
    ring->velocity[0]=lagi::formationAdd(ring->velocity[0].asS32(),(randomNumber()&0xFFFF)-0x7FFF);
    ring->velocity[2]=lagi::formationAdd(ring->velocity[2].asS32(),(randomNumber()&0xFFFF)-0x7FFF);
    ring->remaining=(randomNumber()&0x2F)+30;
    ring->file=dramAllocatorEnd[readSaturnU8(rig.descriptor)].mC_fileBundle;
    return ring;
}
