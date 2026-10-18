// Reuse the existing Azel service mocks; execute both production task bodies.
#define main healing_regression_main
#include "battle_heal_task_test.cpp"
#undef main
int vdp1DrawQuadScaled(sAnimatedQuad* p,sVec3_FP*,fixedPoint)
{
    assert(p->data&&p->vram==0x100);++emissions;return 1;
}
#include "lagi_battle_status_sparkles.inc"
int main()
{
    sVec3_FP dragon{};
    rngCalls=quadInits=quadUpdates=emissions=fadeCalls=0;
    engine.m188_flags.m100_attackAnimationFinished=0;
    lagiCreateStatusSparkles(&engine,&dragon,{0x060AC000},2);
    auto* task=static_cast<sNativeStatusSparkles*>(created);
    assert(task&&rngCalls==80&&quadInits==8&&task->cycles==2);
    for(auto& p:task->particles){assert(p.delay<12&&p.motion.scale==0x8000&&p.motion.radialAcceleration==0x106);}
    animationEnd=true;
    unsigned iterations=0;
    while(!task->control.finished){
        assert(++iterations<200);
        lagiStatusSparklesUpdate(task);
        lagiStatusSparklesDraw(task);
        assert(matrixDepth==0);
        assert(engine.m188_flags.m100_attackAnimationFinished==0&&fadeCalls==0);
    }
    assert(task->completedCycles==2&&quadUpdates>0&&emissions>0);
    lagiStatusSparklesDelete(task);assert(task->quads==nullptr);delete task;
    puts("PASS: production status-sparkle initialization/RNG, delayed animation, two cycles, drawing and cleanup; independent of model/command completion. Not hardware validation.");
}
