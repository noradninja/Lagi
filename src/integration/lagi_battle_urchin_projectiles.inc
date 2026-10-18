p_workArea lagiCreateBattleProjectile(const sVec3_FP&,const sVec3_FP&,
    const sLagiBattleProjectileDescriptor&);

// Native 06056604 monitors the dragon-owned projectile tasks until all finish.
// The emitter is enemy-owned; cancelling it must not destroy those projectiles.
struct sLagiUrchinProjectileMonitor:s_workAreaTemplate<sLagiUrchinProjectileMonitor>
{
    std::vector<p_workArea>* projectiles{};
};
static void lagiUrchinProjectileMonitorUpdate(sLagiUrchinProjectileMonitor* task)
{
    bool active=false;
    for(auto& projectile:*task->projectiles){
        if(projectile && projectile->getTask()->isFinished())projectile=nullptr;
        if(projectile)active=true;
    }
    if(!active)task->getTask()->markFinished();
}
static void lagiUrchinProjectileMonitorDelete(sLagiUrchinProjectileMonitor* task)
{
    delete task->projectiles;task->projectiles=nullptr;
}
p_workArea lagiCreateUrchinProjectileEffect(s_workAreaCopy* parent,bool alternate,bool strong)
{
    static const sLagiUrchinProjectileMonitor::TypedTaskDefinition definition={
        nullptr,lagiUrchinProjectileMonitorUpdate,nullptr,lagiUrchinProjectileMonitorDelete};
    auto* task=createSubTask<sLagiUrchinProjectileMonitor>(parent,&definition);
    if(!task)return nullptr;
    task->projectiles=new std::vector<p_workArea>();
    task->projectiles->reserve(alternate?(strong?16:8):(strong?20:8));
    lagiBuildUrchinProjectilePattern(static_cast<sUrchin*>(parent)->m8,
        gBattleManager->m10_battleOverlay->m18_dragon->m8_position,alternate,strong,
        [&](const sVec3_FP& source,const sVec3_FP& target,const sLagiBattleProjectileDescriptor& d){
            task->projectiles->push_back(lagiCreateBattleProjectile(source,target,d));
        });
    playSystemSoundEffect(0x68);
    return task;
}
p_workArea lagiCreateUrchinSingleProjectile(s_workAreaCopy* parent)
{
    playSystemSoundEffect(0x69);
    return lagiCreateBattleProjectile(static_cast<sUrchin*>(parent)->m8,
        gBattleManager->m10_battleOverlay->m18_dragon->m8_position,
        lagiReadBattleProjectileDescriptor(g_BTL_A3->getSaturnPtr(0x060A7B90)));
}
sVec3_FP* lagiUrchinEffectPosition(s_workAreaCopy* parent)
{
    return &static_cast<sUrchin*>(parent)->m8;
}
