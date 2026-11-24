// Original 060540E2 creates a child, samples the endpoint and computes one
// reciprocal per axis before fixed-point multiplication. No endpoint snap.
struct LagiNativeVectorTransition : s_workAreaTemplate<LagiNativeVectorTransition> {
    sVec3_FP velocity;
    sVec3_FP* live=nullptr;
    s16 remaining=0;
    static void Update(LagiNativeVectorTransition* self) {
        for (unsigned axis=0;axis<3;++axis)
            (*self->live)[axis]=lagi::formationAdd((*self->live)[axis].asS32(),self->velocity[axis].asS32());
        const auto bits=u16(u16(self->remaining)-1);
        self->remaining=s16(bits<0x8000u?s32(bits):s32(bits)-0x10000);
        if (self->remaining<=0) self->getTask()->markFinished();
    }
    static TypedTaskDefinition* getTypedTaskDefinition() {
        static TypedTaskDefinition definition={nullptr,&Update,nullptr,nullptr};
        return &definition;
    }
};
s_workArea* lagiCreateNativeVectorTransition(s_workArea* parent,sVec3_FP* live,
    const sVec3_FP& endpoint,std::int16_t frames) {
    auto* task=createSubTask<LagiNativeVectorTransition>(parent);
    if (!task) return nullptr;
    for (unsigned axis=0;axis<3;++axis) {
        const auto delta=lagi::formationSubtract(endpoint[axis].asS32(),(*live)[axis].asS32());
        // Native calls the hardware divide separately for each axis.
        const auto reciprocal=FP_Div(0x10000,fixedPoint(s32(frames)*0x10000));
        task->velocity[axis]=MTH_Mul(reciprocal,fixedPoint(delta));
    }
    task->live=live;task->remaining=frames;
    return task;
}
