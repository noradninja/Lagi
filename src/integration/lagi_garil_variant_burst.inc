// BTL_A5 06054384 / 060544DC / 060545A4. Timed ballistic burst.
// All visuals share one quad animation, advanced once per Draw, not Update.
struct LagiGarilVariantBurst : s_workAreaTemplate<LagiGarilVariantBurst> {
    struct Visual {sVec3_FP position,velocity;u16 randomWord=0;bool active=true;};
    std::array<Visual,31> visuals;
    sAnimatedQuad quad;
    s16 activeTimer=27,tailTimer=128;
    u8 hitTimer=0;
    bool active=true,tail=false;
    static s16 Word(s32 value) {u16 bits=u16(u32(value));s16 result;std::memcpy(&result,&bits,2);return result;}
    static void Update(LagiGarilVariantBurst* self) {
        auto* dragon=gBattleManager->m10_battleOverlay->m18_dragon;
        if(self->active) {
            const auto old=self->activeTimer;
            self->activeTimer=Word(s32(old)-1);
            if(old<0){self->active=false;self->tail=true;}
            if(self->activeTimer==self->hitTimer) {
                const sVec3_FP zero={0,0,0};
                applyDamageToDragon(dragon->m8C,44,dragon->m8_position,2,zero,0x0800);
            }
        } else if(self->tail) {
            const auto old=self->tailTimer;
            self->tailTimer=Word(s32(old)-1);
            if(old<0){dragon->m8C.m4C=0x1000;self->getTask()->markFinished();}
        }
        for(auto& visual:self->visuals)if(visual.active) {
            visual.velocity[1]=lagi::formationAdd(visual.velocity[1].asS32(),-0xB0);
            for(unsigned axis=0;axis<3;++axis)
                visual.position[axis]=lagi::formationAdd(visual.position[axis].asS32(),visual.velocity[axis].asS32());
        }
    }
    static void Draw(LagiGarilVariantBurst* self) {
        for(auto& visual:self->visuals)if(visual.active)
            drawProjectedParticle(&self->quad,&visual.position);
        sGunShotTask_UpdateSub4(&self->quad);
    }
    static TypedTaskDefinition* getTypedTaskDefinition() {
        static TypedTaskDefinition definition={nullptr,&Update,&Draw,nullptr};return &definition;
    }
};
static void lagiCreateGarilVariantBurst(s_workAreaCopy* owner,const sVec3_FP& source,const sVec3_FP& target) {
    auto* burst=createSubTask<LagiGarilVariantBurst>(owner);
    if(!burst)return;
    sVec3_FP delta,velocity;
    for(unsigned axis=0;axis<3;++axis)
        delta[axis]=lagi::formationSubtract(target[axis].asS32(),source[axis].asS32());
    const auto length=sqrt_F(MTH_Product3d_FP(delta,delta));
    for(unsigned axis=0;axis<3;++axis)
        velocity[axis]=MTH_Mul(FP_Div(0x1000,length),delta[axis]);
    auto* file=dramAllocatorEnd[0xE].mC_fileBundle;
    particleInitSub(&burst->quad,file->m4_vd1Allocation->m4_vdp1Memory,
        lagiNativeBattleParticleQuad(g_BTL_A5->getSaturnPtr(0x060AD2D8)));
    const auto scrollZ=gBattleManager->m10_battleOverlay->m4_battleEngine->m1A0_battleAutoScrollDelta[2].asS32();
    for(auto& visual:burst->visuals) {
        visual.position=source;
        for(unsigned axis=0;axis<3;++axis)
            visual.velocity[axis]=MTH_Mul(0x28000,velocity[axis]);
        visual.velocity[0]=lagi::formationAdd(visual.velocity[0].asS32(),(randomNumber()&0x1FFF)-0xFFF);
        visual.velocity[1]=lagi::formationAdd(visual.velocity[1].asS32(),(randomNumber()&0x1FFF)-0xFFF);
        visual.velocity[2]=lagi::formationAdd(visual.velocity[2].asS32(),(randomNumber()&0x7FF)-0x3FF);
        visual.velocity[2]=lagi::formationAdd(visual.velocity[2].asS32(),scrollZ);
        visual.randomWord=randomNumber()&0x7F;
    }
    burst->hitTimer=(randomNumber()&0xE)+13;
}
static void lagiFireGarilVariantBurst(sBTL_A5_VariantEnemy* actor) {
    // Native model+54 points at 0x64-byte targetables: 0x1F4 = target 5.
    const auto& model=actor->m2C_models[0];
    assert(model.m54_targetables && model.m52_targetableCount>5 && model.m54_targetables[5].m8);
    if(!model.m54_targetables || model.m52_targetableCount<=5 || !model.m54_targetables[5].m8)return;
    lagiCreateGarilVariantBurst(actor,*model.m54_targetables[5].m8,
        gBattleManager->m10_battleOverlay->m18_dragon->mF0);
}
