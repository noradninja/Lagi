// CAMP0605669C, definition0607B270, Update060569EC/Draw060570F0.
// Included by generated campDragon.cpp. Keep Camp's resources, expiry and
// ownership separate from the similar battle heal effect.
#include "lagi/battle_effect_motion.h"
#include "kernel/vdp1AnimatedQuad.h"
#include "kernel/vdp1Allocator.h"
#include "mainMenuDebugTasks.h"

int lagiDrawScaledParticleWithColorTable(sAnimatedQuad*,sVec3_FP*,fixedPoint,sSaturnPtr);

struct sCampRecoveryEffect : s_workAreaTemplateWithCopy<sCampRecoveryEffect>
{
    struct Particle { lagi::battle::effects::ParticleSeed seed; sAnimatedQuad quad; };
    lagi::battle::effects::Motion* motions{};
    Particle* particles{};
    std::vector<sVdp1Quad>* quads{};
    sSaturnPtr colors;
    sVec3_FP* position{};
    s32 typeCount{},particleCount{};
    u16 frame{};
    fixedPoint scale;
    s32 finalArgument{}; // Native+34 is stored but unused by Update/Draw.
};

static void lagiCampRecoveryEffectDelete(sCampRecoveryEffect* task)
{
    delete task->quads;
    task->quads=nullptr;
    // The task heap owns the POD arrays, including on parent teardown.
}

static void lagiCampRecoveryEffectUpdate(sCampRecoveryEffect* task)
{
    using namespace lagi::battle::effects;
    ++task->frame;
    if(task->frame<70)
        advanceMotionRing(std::span<Motion>(task->motions,task->typeCount),
            [](){return randomNumber();},[](u32 a){return getSin(a).asS32();},
            [](u32 a){return getCos(a).asS32();});
    const auto nativeTarget=readSaturnVec3(gCurrentTownOverlay->getSaturnPtr(0x0607B264));
    const std::array<s32,3> target={nativeTarget[0].asS32(),nativeTarget[1].asS32(),nativeTarget[2].asS32()};
    for(s32 i=0;i<task->particleCount;++i) {
        auto& p=task->particles[i];
        if(task->frame<p.seed.delay)continue;
        if(task->frame==70)p.seed.state=1;
        advanceParticleMotion(p.seed.motion,target,p.seed.state);
        sGunShotTask_UpdateSub4(&p.quad);
    }
}

static void lagiCampRecoveryEffectDraw(sCampRecoveryEffect* task)
{
    s32 completed=0;
    for(s32 i=0;i<task->particleCount;++i) {
        auto& p=task->particles[i];
        if(task->frame<p.seed.delay)continue;
        if(task->frame>p.seed.delay+100) {
            if(++completed!=task->particleCount)continue;
            // The original draws the final expired particle on its last call.
            task->getTask()->markFinished();
        }
        pushCurrentMatrix();
        translateCurrentMatrix(*task->position);
        const auto& angles=task->motions[p.seed.type].angles;
        sVec3_FP rotation={angles[0],angles[1],angles[2]};
        rotateCurrentMatrixYXZ(rotation);
        const auto& position=p.seed.motion.position;
        sVec3_FP offset={position[0],position[1],position[2]};
        lagiDrawScaledParticleWithColorTable(&p.quad,&offset,task->scale,task->colors);
        popMatrix();
    }
}

static void lagiCreateCampRecoveryEffect(s_workAreaCopy* parent,sSaturnPtr colors,
    sVec3_FP* position,s32 radius,s32 typeCount,s32 particleCount,s32 scale,s32 finalArgument)
{
    static const sCampRecoveryEffect::TypedTaskDefinition definition={nullptr,
        &lagiCampRecoveryEffectUpdate,&lagiCampRecoveryEffectDraw,&lagiCampRecoveryEffectDelete};
    // Native060568F6 selects file entry2 (+20, native stride10), not battle's0.
    assert(typeCount>0 && particleCount>0 && position && dramAllocatorEnd.size()>2);
    auto* task=createSubTaskWithCopy<sCampRecoveryEffect>(parent,&definition);
    task->typeCount=typeCount;task->particleCount=particleCount;
    task->colors=colors;task->position=position;task->scale=scale;
    task->finalArgument=finalArgument;task->frame=0;
    task->motions=static_cast<lagi::battle::effects::Motion*>(allocateHeapForTask(task,sizeof(*task->motions)*typeCount));
    task->particles=static_cast<sCampRecoveryEffect::Particle*>(allocateHeapForTask(task,sizeof(*task->particles)*particleCount));
    task->quads=new std::vector<sVdp1Quad>(initVdp1Quad(gCurrentTownOverlay->getSaturnPtr(0x06073CA8)));
    if(!task->motions || !task->particles || task->quads->empty()) {
        task->m_UpdateMethod=nullptr;task->m_DrawMethod=nullptr;
        task->getTask()->markFinished();return;
    }
    const auto vram=dramAllocatorEnd[2].mC_fileBundle->m4_vd1Allocation->m4_vdp1Memory;
    for(s32 i=0;i<particleCount;++i) {
        // The native factory reinitializes the entire motion ring for EVERY
        // particle. Preserve the overwritten states and all RNG consumption.
        for(s32 type=0;type<typeCount;++type)
            new (&task->motions[type]) lagi::battle::effects::Motion(
                lagi::battle::effects::initializeMotion([](){return randomNumber();},
                    [](u32 a){return getSin(a).asS32();},[](u32 a){return getCos(a).asS32();}));
        auto& p=*new (&task->particles[i]) sCampRecoveryEffect::Particle{};
        p.seed=lagi::battle::effects::initializeParticleSeed(typeCount,radius,
            [](){return randomNumber();},[](u32 a){return getSin(a).asS32();},
            [](u32 a){return getCos(a).asS32();});
        // Native quad init lies between the seed's first3 and final4 random
        // draws; particleInitSub only initializes quad fields and consumes no RNG.
        particleInitSub(&p.quad,vram,task->quads);
    }
}
