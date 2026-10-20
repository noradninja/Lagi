// Native BTL_A3_2 0605A4EC. Both task definitions share simulation;
// mode 1 selects 060A0DE4 (per-segment Gouraud trail rendering).
void lagiDrawBattleTrailGouraud(sTrailRenderer*);
static void lagiBileDelete(sBileProjectile* task)
{
    delete[] task->m2C_trail.m0_positions;
    task->m2C_trail.m0_positions = nullptr;
}
static void lagiBileGouraudDraw(sBileProjectile* task)
{
    lagiDrawBattleTrailGouraud(&task->m2C_trail);
}
static void lagiCreateArachnothBile(s_workAreaCopy* parent, sVec3_FP* source, s32 speed, s32 mode)
{
    static const sBileProjectile::TypedTaskDefinition plain = {
        nullptr, bileProjectile_update, bileProjectile_draw, lagiBileDelete};
    static const sBileProjectile::TypedTaskDefinition shaded = {
        nullptr, bileProjectile_update, lagiBileGouraudDraw, lagiBileDelete};
    assert(mode == 0 || mode == 1);
    auto* task = createSubTaskWithCopy<sBileProjectile>(parent, mode ? &shaded : &plain);
    if (!task) return;
    const auto descriptor = g_BTL_A3_2->getSaturnPtr(0x060A9624);
    auto* owner = dramAllocatorEnd[readSaturnS32(descriptor)].mC_fileBundle;
    task->m4_vd1Allocation = owner->m4_vd1Allocation;
    task->m8_acceleration.zeroize();
    task->m20_position = *source;
    const s32 pitch = MTH_Mul(static_cast<u32>(randomNumber()) >> 16, 0x08000000) - 0x08000000;
    const s32 yaw = randomNumber() & 0x0FFFFFFF;
    const s32 cosPitch = getCos((pitch >> 16) & 0xFFF);
    task->m14_velocity[0] = MTH_Mul_5_6(cosPitch, getSin((yaw >> 16) & 0xFFF), speed);
    task->m14_velocity[1] = MTH_Mul(-getSin((pitch >> 16) & 0xFFF), speed);
    task->m14_velocity[2] = MTH_Mul_5_6(cosPitch, getCos((yaw >> 16) & 0xFFF), speed);
    trailRenderer_init(&task->m2C_trail, task, source,
        task->m4_vd1Allocation->m4_vdp1Memory, descriptor);
    task->m44_lifetime = 0x78;
}
