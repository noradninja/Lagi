// Original E011 06058884..06058BBA. These tasks borrow their native file
// owner's bundle/VRAM; no runtime resource is owned by the script dispatcher.
static s32 lagiE011Remaining() { return e006_scriptFunction_6057438(); }
static s32 lagiE011EffectTicks(s16& previous)
{
    if (e006_scriptFunction_605861eSub0()) return 1;
    const s32 remaining = lagiE011Remaining();
    const s32 delta = previous - remaining;
    // Original tasks sample again when recording the next previous time.
    previous = static_cast<s16>(lagiE011Remaining());
    assert(delta >= 0);
    return delta;
}

struct sE011ScheduledQuad : s_workAreaTemplateWithCopy<sE011ScheduledQuad>
{
    sSaturnPtr descriptor;
    std::vector<sVdp1Quad>* frames{};
    sAnimatedQuad quad;
    sVec3_FP position;
    s16 cycles{}, previous{};
    u8 state{};
};
static void lagiE011QuadDelete(sE011ScheduledQuad* task)
{
    delete task->frames;
    task->frames = nullptr;
}
static void lagiE011QuadUpdate(sE011ScheduledQuad* task)
{
    // 06058964 jumps to the TST/BF-S at 060589BA before the first body.
    for (s32 ticks = lagiE011EffectTicks(task->previous); ticks > 0; --ticks) {
        switch (task->state) {
        case 0:
            particleInitSub(&task->quad, task->m4_vd1Allocation->m4_vdp1Memory, task->frames);
            task->state = 1;
            break;
        case 1:
            if ((sGunShotTask_UpdateSub4(&task->quad) & 2) && --task->cycles <= 0)
                task->state = 2;
            break;
        case 2: task->getTask()->markFinished(); break;
        default: assert(0);
        }
    }
}
static void lagiE011QuadDraw(sE011ScheduledQuad* task)
{
    drawProjectedParticle(&task->quad, &task->position);
}
static p_workArea lagiE011CreateScheduledQuad(sSaturnPtr descriptor)
{
    static const sE011ScheduledQuad::TypedTaskDefinition definition = {
        nullptr, lagiE011QuadUpdate, lagiE011QuadDraw, lagiE011QuadDelete};
    auto* owner = dramAllocatorEnd[readSaturnS16(descriptor)].mC_fileBundle;
    auto* task = createSubTaskWithCopy<sE011ScheduledQuad>(owner, &definition);
    if (!task) return nullptr;
    task->descriptor = descriptor;
    task->position = readSaturnVec3(descriptor + 4);
    task->frames = new std::vector<sVdp1Quad>(initVdp1Quad(readSaturnEA(descriptor + 0x10)));
    task->cycles = readSaturnS16(descriptor + 0x14);
    task->previous = static_cast<s16>(lagiE011Remaining() + 1);
    return task;
}

struct sE011ScheduledModel : s_workAreaTemplateWithCopy<sE011ScheduledModel>
{
    s_3dModel* model{};
    sVec3_FP position, rotation;
    s16 cycles{}, previous{};
    u8 state{};
};
static void lagiE011ModelDelete(sE011ScheduledModel* task)
{
    // Work areas have no virtual C++ destructor in the retained upstream API.
    // Explicitly release this task's pose/matrix vectors, not the file cache.
    delete task->model;
    task->model = nullptr;
}
static void lagiE011ModelUpdate(sE011ScheduledModel* task)
{
    // 06058A5E likewise enters through the test, not the update body.
    for (s32 ticks = lagiE011EffectTicks(task->previous); ticks > 0; --ticks) {
        switch (task->state) {
        case 0: task->state = 1; [[fallthrough]];
        case 1: {
            const s32 frame = static_cast<s16>(stepAnimation(task->model));
            const s32 count = task->model->m30_pCurrentAnimation
                ? task->model->m30_pCurrentAnimation->m4_numFrames : 0;
            if (frame == count - 1 && --task->cycles <= 0) task->state = 2;
            break;
        }
        case 2: task->getTask()->markFinished(); break;
        default: assert(0);
        }
    }
}
static void lagiE011ModelDraw(sE011ScheduledModel* task)
{
    pushCurrentMatrix();
    translateCurrentMatrix(task->position);
    rotateCurrentMatrixZYX(task->rotation);
    task->model->m18_drawFunction(task->model);
    popMatrix();
}
static p_workArea lagiE011CreateScheduledModel(sSaturnPtr descriptor)
{
    static const sE011ScheduledModel::TypedTaskDefinition definition = {
        nullptr, lagiE011ModelUpdate, lagiE011ModelDraw, lagiE011ModelDelete};
    auto* owner = dramAllocatorEnd[readSaturnS16(descriptor)].mC_fileBundle;
    auto* task = createSubTaskWithCopy<sE011ScheduledModel>(owner, &definition);
    if (!task) return nullptr;
    task->model = new s_3dModel{};
    task->position = readSaturnVec3(descriptor + 4);
    task->rotation = readSaturnVec3(descriptor + 0x10);
    task->cycles = readSaturnS16(descriptor + 0x22);
    task->previous = static_cast<s16>(lagiE011Remaining() + 1);
    auto* bundle = task->m0_fileBundle;
    const u16 hierarchyIndex = readSaturnU16(descriptor + 0x1C);
    auto* hierarchy = bundle->getModelHierarchy(hierarchyIndex);
    if (!hierarchy || !init3DModelRawData(task, task->model, 0, bundle, hierarchyIndex,
        bundle->getAnimation(readSaturnU16(descriptor + 0x1E)),
        bundle->getStaticPose(readSaturnU16(descriptor + 0x20), hierarchy->countNumberOfBones()),
        nullptr, nullptr)) {
        task->m_UpdateMethod = nullptr;
        task->m_DrawMethod = nullptr;
        task->getTask()->markFinished();
    }
    return task;
}

// The scheduler and recovered ribbon callback are in the adjacent
// lagi_e011_ribbon_scheduler.inc; these leaf tasks retain independent tests.
