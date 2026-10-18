// BTL_A3 060569A8..06056B0E. Animation-owned emitter requested by
// invokeCreateEffect(06056AF2), not an encounter-specific crash bypass.
struct sNativeUrchinAmbient : public s_workAreaTemplate<sNativeUrchinAmbient>
{
    sVec3_FP* position{};
};

static void lagiUrchinAmbientUpdate(sNativeUrchinAmbient* task)
{
    if (randomNumber() & 7u) return;
    auto angle = []() {
        const u32 sign = randomNumber();
        const u32 value = randomNumber();
        const u32 raw = (sign & 0x08000000u)
            ? (value | 0xF0000000u) : (value & 0x0FFFFFFFu);
        return (raw >> 16) & 0xFFFu;
    };
    const u32 a = angle();
    const u32 b = angle();
    // Original 20216E80 is sine, 20217E80 cosine (COMMON 00216E80
    // starts at zero). Native Y reuses radial; do not substitute a sphere.
    const fixedPoint radial = MTH_Mul(fixedPoint(0xA000), getSin(a));
    sVec3_FP position;
    position[0] = (*task->position)[0] + MTH_Mul(radial, getSin(b));
    position[1] = (*task->position)[1] + radial;
    position[2] = (*task->position)[2] + MTH_Mul(radial, getCos(b));

    const s32 selected = performModulo(3, randomNumber());
    // Immutable descriptors stay alive after this emitter's animation ends:
    // its already-spawned particles belong to the native slot-12 file task.
    static const std::array<std::vector<sVdp1Quad>, 3> quads = {
        initVdp1Quad(g_BTL_GenericData->getSaturnPtr(0x060A8300)),
        initVdp1Quad(g_BTL_GenericData->getSaturnPtr(0x060A84C0)),
        initVdp1Quad(g_BTL_GenericData->getSaturnPtr(0x060A87B4)),
    };
    createParticleEffect(dramAllocatorEnd[12].mC_fileBundle,
        &quads[selected], &position, nullptr, nullptr, 0x10000, nullptr, 0);
}

p_workArea lagiCreateUrchinAmbientEffect(s_workAreaCopy* parent)
{
    auto* task = createSubTaskFromFunction<sNativeUrchinAmbient>(
        parent, &lagiUrchinAmbientUpdate);
    if (task) task->position = &static_cast<sUrchin*>(parent)->m8;
    return task;
}
