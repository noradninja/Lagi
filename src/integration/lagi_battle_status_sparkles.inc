// Independent native secondary status-cure task, BTL_A3 06099AA0..06099F0A.
// The companion two-model task remains separate; no proxy model is drawn.
struct sNativeStatusSparkles : public s_workAreaTemplateWithCopy<sNativeStatusSparkles>
{
    struct Particle {
        sAnimatedQuad quad;
        lagi::battle::effects::SparkleMotion motion;
        std::array<s32,3> position{};
        u16 delay{};
    };
    std::array<Particle,8> particles;
    std::vector<sVdp1Quad>* quads{};
    sVec3_FP* position{};
    u16 frame{};
    s32 cycles{},completedCycles{};
};

static void lagiReseedStatusSparkle(sNativeStatusSparkles::Particle& p)
{
    lagi::battle::effects::reseedSparkleMotion(p.motion,p.position,
        [](){return randomNumber();},[](u32 a){return getSin(a).asS32();},
        [](u32 a){return getCos(a).asS32();});
}
static void lagiResetStatusSparkle(sNativeStatusSparkles::Particle& p)
{
    lagiReseedStatusSparkle(p);
    p.motion.scale=0x8000;
    p.delay=static_cast<u16>(lagi::battle::effects::multiply(randomNumber() >> 16,12));
}
static void lagiStatusSparklesDelete(sNativeStatusSparkles* task)
{
    delete task->quads;task->quads=nullptr;
}
static void lagiStatusSparklesUpdate(sNativeStatusSparkles* task)
{
    ++task->frame;
    for (auto& p:task->particles) {
        if (task->frame < p.delay) continue;
        lagi::battle::effects::advanceSparkleMotion(p.motion);
        const auto flags=sGunShotTask_UpdateSub4(&p.quad);
        lagi::battle::effects::finishSparkleAnimationStep(p.motion,flags,
            [](){return randomNumber();},[&](){lagiReseedStatusSparkle(p);});
    }
}
static void lagiStatusSparklesDraw(sNativeStatusSparkles* task)
{
    unsigned expired=0;
    for (auto& p:task->particles) {
        if (task->frame < p.delay) continue;
        if (p.motion.scale<=0) {
            if (++expired!=task->particles.size()) continue;
            if (++task->completedCycles>=task->cycles)
                task->getTask()->markFinished();
            else {
                for (auto& restart:task->particles) lagiResetStatusSparkle(restart);
                task->frame=0;
            }
        }
        // Native 06099D84 overwrites local Y with current radius before
        // translating to the dragon and applying this particle's Y rotation.
        p.position[1]=p.motion.radius;
        sVec3_FP local={p.position[0],p.position[1],p.position[2]};
        sVec3_FP rotation={0,p.motion.angle,0};
        pushCurrentMatrix();
        translateCurrentMatrix(*task->position);
        rotateCurrentMatrixYXZ(rotation);
        vdp1DrawQuadScaled(&p.quad,&local,fixedPoint(p.motion.scale));
        popMatrix();
    }
}
static void lagiCreateStatusSparkles(s_battleEngine* engine,sVec3_FP* position,
    sSaturnPtr quadData,s32 cycles)
{
    static const sNativeStatusSparkles::TypedTaskDefinition definition={nullptr,
        &lagiStatusSparklesUpdate,&lagiStatusSparklesDraw,&lagiStatusSparklesDelete};
    auto* task=createSubTaskWithCopy<sNativeStatusSparkles>(engine,&definition);
    if (!task) return;
    task->position=position;task->cycles=cycles;task->frame=0;task->completedCycles=0;
    task->quads=new std::vector<sVdp1Quad>(initVdp1Quad(quadData));
    if (!position || task->quads->empty()) {
        task->m_UpdateMethod=nullptr;task->m_DrawMethod=nullptr;
        task->getTask()->markFinished();return;
    }
    const auto vram=dramAllocatorEnd[0].mC_fileBundle->m4_vd1Allocation->m4_vdp1Memory;
    for (auto& p:task->particles) {
        particleInitSub(&p.quad,vram,task->quads);
        lagiResetStatusSparkle(p);
    }
}
