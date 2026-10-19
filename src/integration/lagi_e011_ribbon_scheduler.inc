// E011 06058BBC/06058F04/06058FF8. Data and VRAM remain owned by the
// descriptor's native resource owner; the scheduler owns no copied game assets.
void lagiDisplayWorldStrip(std::array<sVec3_FP,2>&,std::array<fixedPoint,2>&,
    u16,s16,u16,const quadColor*,s32);
struct sE011Ribbon : s_workAreaTemplateWithCopy<sE011Ribbon>
{
    sSaturnPtr descriptor;
    std::array<sVec3_FP,20> history;
    std::array<u16,20> colors;
    sVec3_FP start,end,amplitude,tangent;
    fixedPoint remaining;
    s16 previous{};
    u8 state{},tail{};
};
static void lagiE011RibbonUpdate(sE011Ribbon* task)
{
    for(s32 ticks=lagiE011EffectTicks(task->previous);ticks>0;--ticks) {
        switch(task->state) {
        case 0: {
            const s16 sound=readSaturnS16(task->descriptor+0x42);
            if(sound>=0)playSystemSoundEffect(sound);
            task->state=1;
            [[fallthrough]];
        }
        case 1:
            for(int i=19;i>0;--i)task->history[i]=task->history[i-1];
            if(task->remaining>0) {
                const auto duration=readSaturnFP(task->descriptor+0x20);
                const auto t=FP_Div(duration-task->remaining,duration);
                const auto wave=getSin(t.asS32()>>4);
                for(int axis=0;axis<3;++axis)
                    task->history[0][axis]=task->start[axis]+
                        MTH_Mul(t,task->end[axis]-task->start[axis])+MTH_Mul(wave,task->amplitude[axis]);
                task->remaining-=0x10000;
            } else {
                task->history[0]=task->end;
                task->tangent=task->history[0]-task->history[1];
                task->tail=30;
                task->state=2;
            }
            break;
        case 2:
            for(int i=19;i>0;--i)task->history[i]=task->history[i-1];
            task->history[0]+=task->tangent;
            if(--task->tail==0)task->state=3;
            break;
        case 3: task->getTask()->markFinished();break;
        default: break;
        }
    }
}
static void lagiE011RibbonDraw(sE011Ribbon* task)
{
    const u16 base=task->m4_vd1Allocation->m4_vdp1Memory;
    for(unsigned i=0;i<19;++i) {
        std::array<sVec3_FP,2> points={task->history[i],task->history[i+1]};
        const auto widthTable=gTWN_E011->getSaturnPtr(0x06061F4C+i*8);
        std::array<fixedPoint,2> widths={readSaturnFP(widthTable),readSaturnFP(widthTable+4)};
        const s8 selector=readSaturnS8(gTWN_E011->getSaturnPtr(0x06061F38+i));
        const auto sprite=task->descriptor+0x24+selector*6;
        quadColor colors={task->colors[i],task->colors[i],task->colors[i+1],task->colors[i+1]};
        lagiDisplayWorldStrip(points,widths,base+readSaturnU16(sprite),
            readSaturnS16(sprite+2),base+readSaturnU16(sprite+4),&colors,8);
    }
}
static p_workArea lagiE011CreateRibbon(sSaturnPtr descriptor)
{
    static const sE011Ribbon::TypedTaskDefinition definition={nullptr,lagiE011RibbonUpdate,lagiE011RibbonDraw,nullptr};
    auto* owner=dramAllocatorEnd[readSaturnS16(descriptor)].mC_fileBundle;
    auto* task=createSubTaskWithCopy<sE011Ribbon>(owner,&definition);
    if(!task)return nullptr;
    task->descriptor=descriptor;
    task->start=readSaturnVec3(descriptor+4);
    task->end=readSaturnVec3(descriptor+0x10);
    task->remaining=readSaturnFP(descriptor+0x20);
    task->previous=static_cast<s16>(lagiE011Remaining()+1);
    sVec3_FP rotation;
    computeLookAt(task->end-task->start,rotation);
    rotation[2]=readSaturnFP(descriptor+0x1C);
    const sVec3_FP offset(0,performModulo(0x6000,static_cast<s32>(randomNumber()))+0x3000,0);
    sMatrix4x3 matrix;
    initMatrixToIdentity(&matrix);
    rotateMatrixYXZ(&rotation,&matrix);
    transformAndAddVec(offset,task->amplitude,matrix);
    for(unsigned i=0;i<20;++i) {
        task->history[i]=task->start;
        const auto t=intDivide(19,i*0x10000);
        u16 color=0x8000;
        for(unsigned channel=0;channel<3;++channel) {
            const s32 a=readSaturnU16(descriptor+0x36+channel*2);
            const s32 b=readSaturnU16(descriptor+0x3C+channel*2);
            color|=static_cast<u16>((MTH_Mul(a,0x10000-t.asS32())+MTH_Mul(b,t))<<(channel*5));
        }
        task->colors[i]=color;
    }
    return task;
}

// 06058884: dispatch every threshold already crossed this frame, in table
// order. End sentinel and cinematic stop finish the scheduler, not its owner.
struct sE011Schedule : s_workAreaTemplate<sE011Schedule>
{
    sSaturnPtr table;
    static void Update(sE011Schedule* task) {
        if(readSaturnS32(task->table)<=0 || e006_scriptFunction_605861eSub0()) {
            task->getTask()->markFinished();return;
        }
        // Native samples the clock once, before the catch-up loop.
        const s32 remaining=lagiE011Remaining();
        while(remaining<=readSaturnS32(task->table)) {
            // Native 060588E2..E6: argument at +4, function at +8.
            const auto argument=readSaturnEA(task->table+4);
            const auto callback=readSaturnEA(task->table+8);
            switch(callback.m_offset) {
            case 0x060589E2: lagiE011CreateScheduledQuad(argument);break;
            case 0x06058B2A: lagiE011CreateScheduledModel(argument);break;
            case 0x06058FF8: lagiE011CreateRibbon(argument);break;
            default: assert(0);
            }
            task->table=task->table+12;
        }
    }
};
static s32 lagiE011StartSchedule(s32 argument)
{
    static const sE011Schedule::TypedTaskDefinition definition={nullptr,sE011Schedule::Update,nullptr,nullptr};
    auto* task=createSubTask<sE011Schedule>(currentResTask,&definition);
    if(!task)return 0;
    task->table=readSaturnEA(gTWN_E011->getSaturnPtr(argument));
    return static_cast<s32>(reinterpret_cast<uintptr_t>(task));
}
