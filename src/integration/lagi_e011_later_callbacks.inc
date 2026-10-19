// 06057F00 / 0605809E. Timed native CRAM sequence; descriptors remain in
// the live E011 overlay and the child follows currentResTask's lifetime.
struct sE011PaletteSequence : s_workAreaTemplate<sE011PaletteSequence>
{
    sSaturnPtr descriptor;
    s32 previous{},delay{},index{};
    static void Update(sE011PaletteSequence* task) {
        s32 ticks=1;
        if(!e006_scriptFunction_605861eSub0()) {
            ticks=task->previous-lagiE011Remaining();
            task->previous=lagiE011Remaining();
        }
        for(;ticks>0;--ticks) {
            if(--task->delay>0)continue;
            const auto table=readSaturnEA(task->descriptor+4);
            auto entry=table+task->index*8;
            const u8 flags=readSaturnU8(task->descriptor+8);
            if(!readSaturnEA(entry).m_offset) {
                if(flags&1) {task->getTask()->markFinished();continue;}
                task->index=0;entry=table;
            }
            const u32 bytes=(flags&2)?0x200:0x20;
            asyncDmaCopy(readSaturnEA(entry),getVdp2Cram(readSaturnU16(task->descriptor)*bytes),bytes,0);
            task->delay=readSaturnU16(entry+4);
            ++task->index;
        }
    }
};
static s32 lagiE011StartPaletteSequence(s32 argument)
{
    if(e006_scriptFunction_605861eSub0())return 0;
    static const sE011PaletteSequence::TypedTaskDefinition definition={nullptr,sE011PaletteSequence::Update,nullptr,nullptr};
    auto* task=createSubTask<sE011PaletteSequence>(currentResTask,&definition);
    if(task) {
        task->descriptor=gTWN_E011->getSaturnPtr(argument);
        task->previous=lagiE011Remaining()+1;
    }
    return 0;
}

// MAIN 0600FF78 cancels overlapping persistent transfers. The native host
// uses fixed typed slots, not Saturn's raw linked-list node layout.
static void lagiE011CancelScreenTransfers()
{
    extern sVdpVar1* vdpVar2;
    if(!vdpVar2)return;
    auto* previous=vdpVar2;
    while(auto* transfer=previous->m10_nextTransfert) {
        const auto destination=reinterpret_cast<uintptr_t>(transfer->m8_destination);
        const auto begin=reinterpret_cast<uintptr_t>(getVdp2Vram(0x1F000));
        const auto end=destination+static_cast<u16>(transfer->mC_size)*16;
        if(destination>=begin+0x800)break;
        if(end>begin) {
            previous->m10_nextTransfert=transfer->m10_nextTransfert;
            if(vdpVar3==transfer)vdpVar3=previous;
            transfer->m10_nextTransfert=nullptr;
            transfer->mF_isPending=0;
        } else previous=transfer;
    }
}
static void lagiE011ScreenCommon(unsigned mode,u32 graphics)
{
    auto* task=gE011BackgroundTask;
    assert(task);
    if(task->m60_state!=mode) {
        lagiE011CancelScreenTransfers();
        unpackGraphicsToVDP2(getSaturnPtr(gTWN_E011->getSaturnPtr(graphics)),getVdp2Vram(0x1F000));
    }
    vdp2Controls.m4_pendingVdp2Regs->m20_BGON|=1;
    vdp2Controls.m_isDirty=1;
    task->m74_cutscenePos=lagiE011Remaining();
    task->m60_state=mode;
}
// 060557E0: descriptor is five wave parameters and a duration.
static s32 lagiE011SetScreenWave(s32 argument)
{
    if(e006_scriptFunction_605861eSub0())return 0;
    const auto descriptor=gTWN_E011->getSaturnPtr(argument);
    lagiE011ScreenCommon(1,0x060607D8);
    auto* task=gE011BackgroundTask;
    task->m70_counter=readSaturnS32(descriptor+0x14);
    task->m58_wavePhase=task->m5C_scrollAccum=0;
    task->m44_dltOmega=readSaturnS32(descriptor);
    task->m48_dltAng=readSaturnS32(descriptor+4);
    task->m4C_ratio=readSaturnS32(descriptor+8);
    task->m50_lineSpd=readSaturnS32(descriptor+0xC);
    task->m54_upSpd=readSaturnS32(descriptor+0x10);
    sE011BackgroundTask::initCinematicBarTable(task);
    return 0;
}
// 06055872: descriptor holds a world point, screen offsets and duration.
static s32 lagiE011SetScreenTracking(s32 argument)
{
    if(e006_scriptFunction_605861eSub0())return 0;
    const auto descriptor=gTWN_E011->getSaturnPtr(argument);
    lagiE011ScreenCommon(2,0x06060926);
    auto* task=gE011BackgroundTask;
    task->m70_counter=readSaturnS32(descriptor+0x10);
    // C++ cannot alias big-endian Saturn vectors as native fixedPoint storage.
    // Own the converted point for the whole background task's lifetime.
    task->lagiTrackingPoint=readSaturnVec3(descriptor);
    task->m64_positionPtr=&task->lagiTrackingPoint;
    task->m68_offsetX=readSaturnS16(descriptor+0xC);
    task->m6C_offsetY=readSaturnS16(descriptor+0xE);
    sE011BackgroundTask::initCinematicBarParams(task);
    return 0;
}
