// BTL_A3_2 06062EAA / 06062F5E / 06062FEE: copied transform,
// animated model, and animation-end lifetime. Resources remain bundle-owned.
struct sLagiBattleImpactModel : s_workAreaTemplateWithCopy<sLagiBattleImpactModel>
{
    s_3dModel* model{};
    sVec3_FP position, rotation;
};
static void lagiBattleImpactDelete(sLagiBattleImpactModel* task)
{
    delete task->model;
    task->model = nullptr;
}
static void lagiBattleImpactUpdate(sLagiBattleImpactModel* task)
{
    stepAnimation(task->model);
    const auto* animation = task->model->m30_pCurrentAnimation;
    const s32 lastFrame = animation ? animation->m4_numFrames - 1 : -1;
    if (task->model->m10_currentAnimationFrame >= lastFrame)
        task->getTask()->markFinished();
}
static void lagiBattleImpactDraw(sLagiBattleImpactModel* task)
{
    pushCurrentMatrix();
    translateCurrentMatrix(task->position);
    rotateCurrentMatrixYXZ(task->rotation);
    task->model->m18_drawFunction(task->model);
    popMatrix();
}
static void lagiCreateBattleImpactModel(npcFileDeleter* owner, u16 hierarchyIndex,
    u16 poseIndex, u16 animationIndex, const sVec3_FP& position, const sVec3_FP& rotation)
{
    static const sLagiBattleImpactModel::TypedTaskDefinition definition = {
        nullptr, lagiBattleImpactUpdate, lagiBattleImpactDraw, lagiBattleImpactDelete};
    auto* task = createSubTaskWithCopy<sLagiBattleImpactModel>(owner, &definition);
    if (!task) return;
    task->position = position;
    task->rotation = rotation;
    task->model = new s_3dModel{};
    auto* bundle = task->m0_fileBundle;
    auto* hierarchy = bundle->getModelHierarchy(hierarchyIndex);
    if (!hierarchy || !init3DModelRawData(task, task->model, 0, bundle, hierarchyIndex,
        bundle->getAnimation(animationIndex),
        bundle->getStaticPose(poseIndex, hierarchy->countNumberOfBones()), nullptr, nullptr)) {
        task->m_UpdateMethod = nullptr;
        task->m_DrawMethod = nullptr;
        task->getTask()->markFinished();
    }
}
