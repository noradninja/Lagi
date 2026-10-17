// Native BTL_A3 0608CA18..0608D74A. Included by the generated battle engine;
// Azel owns effect timing/resources, Lagi owns platform emission/storage.
#include "lagi/battle_effect_motion.h"

int lagiDrawScaledParticleWithColorTable(sAnimatedQuad*, sVec3_FP*, fixedPoint, sSaturnPtr);
void urchinFormation_createFadeTask(p_workArea,u16,u16,u16,u16,u16,s16);

struct sHealVisualEffectTask : public s_workAreaTemplateWithCopy<sHealVisualEffectTask>
{
    struct Particle {
        lagi::battle::effects::ParticleSeed seed;
        sAnimatedQuad quad;
    };
    lagi::battle::effects::Motion* motions{};
    Particle* particles{};
    std::vector<sVdp1Quad>* quads{};
    sSaturnPtr colors;
    sVec3_FP* position{};
    s32 typeCount{}, particleCount{};
    u16 frame{};
    fixedPoint scale;
    s32 setCommandFinished{};
};

static void sHealVisualEffectTask_Delete(sHealVisualEffectTask* task)
{
    delete task->quads;
    task->quads=nullptr;
    // POD motion/particle arrays belong to Azel's task heap.
}

static void sHealVisualEffectTask_Update(sHealVisualEffectTask* task)
{
    using namespace lagi::battle::effects;
    ++task->frame;
    if (task->frame < 0x46) {
        advanceMotionRing(std::span<Motion>(task->motions,task->typeCount),
            [](){return randomNumber();},
            [](u32 a){return getSin(a).asS32();},
            [](u32 a){return getCos(a).asS32();});
    }
    if (task->setCommandFinished==1 && task->frame==0x3C) {
        urchinFormation_createFadeTask(task,0xC210,0xE739,0x1E,0xE739,0xC210,10);
        gBattleManager->m10_battleOverlay->m4_battleEngine->m188_flags.m100_attackAnimationFinished=1;
    }
    const auto targetVector=readSaturnVec3(g_BTL_GenericData->getSaturnPtr(0x060AD4C8));
    const std::array<s32,3> target={targetVector[0].asS32(),targetVector[1].asS32(),targetVector[2].asS32()};
    for (s32 i=0;i<task->particleCount;++i) {
        auto& particle=task->particles[i];
        if (task->frame < particle.seed.delay)
            continue;
        if (task->frame==0x46)
            particle.seed.state=1;
        advanceParticleMotion(particle.seed.motion,target,particle.seed.state);
        sGunShotTask_UpdateSub4(&particle.quad);
    }
}

static void sHealVisualEffectTask_Draw(sHealVisualEffectTask* task)
{
    s32 completed=0;
    for (s32 i=0;i<task->particleCount;++i) {
        auto& particle=task->particles[i];
        if (task->frame < particle.seed.delay)
            continue;
        if (task->frame > particle.seed.delay+100 && ++completed==task->particleCount)
            task->getTask()->markFinished();
        pushCurrentMatrix();
        translateCurrentMatrix(*task->position);
        const auto& angles=task->motions[particle.seed.type].angles;
        sVec3_FP rotation={angles[0],angles[1],angles[2]};
        rotateCurrentMatrixYXZ(rotation);
        const auto& p=particle.seed.motion.position;
        sVec3_FP position={p[0],p[1],p[2]};
        lagiDrawScaledParticleWithColorTable(&particle.quad,&position,task->scale,task->colors);
        popMatrix();
    }
}

static void createHealVisualEffect(s_battleEngine* engine,sSaturnPtr colors,
    sVec3_FP* position,s32 radius,s32 typeCount,s32 particleCount,s32 scale,s32 finishCommand)
{
    static const sHealVisualEffectTask::TypedTaskDefinition definition={nullptr,
        &sHealVisualEffectTask_Update,&sHealVisualEffectTask_Draw,&sHealVisualEffectTask_Delete};
    auto fail=[&](){engine->m188_flags.m100_attackAnimationFinished=1;};
    if (typeCount<=0 || particleCount<=0 || !position) {fail();return;}
    auto* task=createSubTaskWithCopy<sHealVisualEffectTask>(engine,&definition);
    if (!task) {fail();return;}
    task->typeCount=typeCount;task->particleCount=particleCount;
    task->colors=colors;task->position=position;task->scale=scale;
    task->setCommandFinished=finishCommand;task->frame=0;
    task->motions=static_cast<lagi::battle::effects::Motion*>(allocateHeapForTask(task,sizeof(*task->motions)*typeCount));
    task->particles=static_cast<sHealVisualEffectTask::Particle*>(allocateHeapForTask(task,sizeof(*task->particles)*particleCount));
    task->quads=new std::vector<sVdp1Quad>(initVdp1Quad(readSaturnEA(g_BTL_GenericData->getSaturnPtr(0x060ABF1C))));
    if (!task->motions || !task->particles || task->quads->empty()) {
        task->m_UpdateMethod=nullptr;task->m_DrawMethod=nullptr;
        task->getTask()->markFinished();fail();return;
    }
    const auto vram=dramAllocatorEnd[0].mC_fileBundle->m4_vd1Allocation->m4_vdp1Memory;
    for (s32 particle=0;particle<particleCount;++particle) {
        // The original initializer overwrites the same type array for every
        // particle. Preserve both the final state and native RNG progression.
        for (s32 type=0;type<typeCount;++type)
            new (&task->motions[type]) lagi::battle::effects::Motion(
                lagi::battle::effects::initializeMotion([](){return randomNumber();},
                    [](u32 a){return getSin(a).asS32();},[](u32 a){return getCos(a).asS32();}));
        auto& p=*new (&task->particles[particle]) sHealVisualEffectTask::Particle{};
        p.seed=lagi::battle::effects::initializeParticleSeed(typeCount,radius,
            [](){return randomNumber();},[](u32 a){return getSin(a).asS32();},
            [](u32 a){return getCos(a).asS32();});
        particleInitSub(&p.quad,vram,task->quads);
    }
}
