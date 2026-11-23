// Shared FLD_A5 06088F2C/06089150 line-effect drawing.
static void lagiAnimRingLine(const sVec3_FP& a,const sVec3_FP& b) {
    std::array<sVec3_FP,2> line{a,b};
    displayRaySegmentFromWorldSpace(line.data(),0,0,0,0xFFFF,8);
}
static sVec3_FP lagiAnimRingPoint(const sVec3_FP& point,const sVec3_FP& direction,u32 angle,bool jitter) {
    sVec3_FP result;
    const auto wave=getCos((angle>>16)&0xFFF);
    for(unsigned axis=0;axis<3;++axis) {
        result[axis]=point[axis]+MTH_Mul(direction[axis],wave);
        if(jitter)result[axis]+=s32(randomNumber()&0x3FFF)-0x2000;
    }
    return result;
}
static void animRingSubTask_Draw(sAnimRingSubTask* task) {
    sVec3_FP step,point=task->m8_center,previous=point;
    for(unsigned axis=0;axis<3;++axis)step[axis]=performDivision(10,
        lagi::formationSubtract(task->m14_current[axis].asS32(),point[axis].asS32()));
    u32 angle=0xBA2E8B;
    for(unsigned segment=0;segment<9;++segment,angle+=0xBA2E8B) {
        point+=step;
        auto next=lagiAnimRingPoint(point,task->m38_direction,angle,true);
        lagiAnimRingLine(previous,next);previous=next;
    }
    lagiAnimRingLine(previous,task->m14_current);
}
static void animRingSubTask_DrawAlt(sAnimRingSubTask* task) {
    const int lifetime=task->m48_lifetime;
    if(lifetime<=0)return;
    sVec3_FP step,point;
    for(unsigned axis=0;axis<3;++axis) {
        const auto delta=lagi::formationSubtract(task->m20_target[axis].asS32(),task->m8_center[axis].asS32());
        step[axis]=performDivision(10,delta);
        const auto product=lagi::formationSignedBits(u32(delta)*u32(task->m44));
        point[axis]=task->m8_center[axis]+fixedPoint(performDivision(lifetime,product));
    }
    const int begin=performDivision(lifetime,s32(u32(task->m44)*10u));
    const int end=begin+3;
    u32 angle=u32(performDivision(lifetime,0x08000000))*u32(task->m44);
    auto previous=lagiAnimRingPoint(point,task->m38_direction,angle,false);
    for(int segment=begin;segment<9 && segment<end;++segment) {
        angle+=0xBA2E8B;point+=step;
        auto next=lagiAnimRingPoint(point,task->m38_direction,angle,true);
        lagiAnimRingLine(previous,next);previous=next;
    }
    if(end!=9)lagiAnimRingLine(previous,task->m20_target);
}
