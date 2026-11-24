// Native BTL_A5_3 06056840 factory / 06056610 Update / 060567F0 Draw.
// A timed hit with five animated spring-following visuals; no contact test.
struct LagiLathumProjectile : s_workAreaTemplate<LagiLathumProjectile> {
    struct Visual {sAnimatedQuad quad;sVec3_FP position,velocity;};
    sVec3_FP position,velocity;
    std::array<Visual,5> visuals;
    s16 remaining=0;
    static void Update(LagiLathumProjectile* self) {
        for(unsigned axis=0;axis<3;++axis)
            self->position[axis]=lagi::formationAdd(self->position[axis].asS32(),self->velocity[axis].asS32());
        for(auto& visual:self->visuals) {
            for(unsigned axis=0;axis<3;++axis) {
                const auto delta=lagi::formationSubtract(self->position[axis].asS32(),visual.position[axis].asS32());
                const auto gain=(randomNumber()&0xFFF)+0x28F;
                visual.velocity[axis]=lagi::formationAdd(visual.velocity[axis].asS32(),MTH_Mul(fixedPoint(gain),fixedPoint(delta)).asS32());
            }
            for(unsigned axis=0;axis<3;++axis)
                visual.position[axis]=lagi::formationAdd(visual.position[axis].asS32(),visual.velocity[axis].asS32());
            sGunShotTask_UpdateSub4(&visual.quad);
        }
        self->remaining=lagiLathumDamageWord(s32(self->remaining)-1);
        if(self->remaining>=0)return;
        auto* dragon=gBattleManager->m10_battleOverlay->m18_dragon;
        sVec3_FP knockback;
        for(unsigned axis=0;axis<3;++axis)
            knockback[axis]=lagi::formationSubtract(dragon->m8_position[axis].asS32(),self->position[axis].asS32());
        applyDamageToDragon(dragon->m8C,28,self->position,3,knockback,0);
        self->getTask()->markFinished();
    }
    static void Draw(LagiLathumProjectile* self) {
        for(auto& visual:self->visuals)vdp1DrawQuadScaled(&visual.quad,&visual.position,0x30000);
    }
    static TypedTaskDefinition* getTypedTaskDefinition() {
        static TypedTaskDefinition definition={nullptr,&Update,&Draw,nullptr};return &definition;
    }
};
static void lagiCreateLathumProjectile(s_workAreaCopy* owner,LagiLathumRig& rig,
    u8 group,s_battleEngine& engine,const sVec3_FP& dragonPosition) {
    auto* projectile=createSubTask<LagiLathumProjectile>(owner);
    if(!projectile)return;
    projectile->remaining=(randomNumber()&31)+75;
    const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+group));
    transformAndAddVec(rig.corePositions[anchor],projectile->position,cameraProperties2.m28[1]);
    projectile->position[1]=lagi::formationAdd(projectile->position[1].asS32(),0xA000);
    rig.cameraTarget=projectile->position;
    for(unsigned axis=0;axis<3;++axis) {
        const auto delta=lagi::formationSubtract(dragonPosition[axis].asS32(),projectile->position[axis].asS32());
        const auto reciprocal=FP_Div(0x10000,fixedPoint(s32(projectile->remaining)*0x10000));
        projectile->velocity[axis]=lagi::formationAdd(MTH_Mul(reciprocal,fixedPoint(delta)).asS32(),engine.m1A0_battleAutoScrollDelta[axis].asS32());
    }
    auto* file=dramAllocatorEnd[readSaturnU8(rig.descriptor)].mC_fileBundle;
    const auto* quads=lagiNativeBattleParticleQuad(g_BTL_A5_3->getSaturnPtr(0x060A3AD8));
    for(unsigned i=0;i<projectile->visuals.size();++i) {
        auto& visual=projectile->visuals[i];
        particleInitSub(&visual.quad,file->m4_vd1Allocation->m4_vdp1Memory,quads);
        for(unsigned frame=0;frame<i;++frame)sGunShotTask_UpdateSub4(&visual.quad);
        visual.position=projectile->position;
        for(unsigned axis=0;axis<3;++axis)
            visual.velocity[axis]=lagi::formationAdd(visual.velocity[axis].asS32(),(randomNumber()&0x1FFF)-0xFFF);
    }
}
