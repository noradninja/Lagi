// Native task descriptor 060AAD98: {null, 06058996, 0609F5B4, null}.
// Reference-bearing views are assembled over this concrete typed work area;
// native task/formation addresses are never treated as host field offsets.
struct sLagiTutorialLaserEnemyTask : s_workAreaTemplateWithCopy<sLagiTutorialLaserEnemyTask> {
    std::vector<s_3dModel> models;
    std::uint8_t modelCount{};
    LagiBattleModelTargetState targets;
    sEnemyLifeMeterTask* lifeMeter{};
    std::uint32_t flags{};
    std::int16_t health{},pendingDamage{},lightColor{},deathTimer{};
    std::int32_t entityIndex{},rate{};
    std::uint8_t state{};
    sVec3_FP position{},currentPosition{},targetPosition{},velocity{},acceleration{};
    sVec3_FP rotation{},currentRotation{},targetRotation{},angularVelocity{},angularAcceleration{};
    sVec3_FP authoredOffset{};
    const sVec3_FP* formationPosition{};
    const sVec3_FP* formationRotation{};
    std::int8_t* deadCount{};
    std::int32_t* experience{};
    std::int32_t* dyne{};
    npcFileDeleter* resources{};
    LagiTutorialLaserEnemyHooks services{};

    LagiBattleModelMotionView motion() {
        return {position,currentPosition,targetPosition,velocity,acceleration,
            rotation,currentRotation,targetRotation,angularVelocity,angularAcceleration,
            *formationPosition,*formationRotation,rate};
    }
    LagiTutorialLaserEnemyView view() {
        return {motion(),{this,flags,health,targets.healthFloor,pendingDamage,lightColor,
            lifeMeter->m31,position,models,targets},state,deathTimer,authoredOffset,
            *deadCount,*experience,*dyne,resources};
    }
    // Each wrapper keeps the actor identity distinct from the caller's service
    // context. In particular two enemies never share a drawing-disable target.
    static sLagiTutorialLaserEnemyTask& self(void* context) {
        return *static_cast<sLagiTutorialLaserEnemyTask*>(context);
    }
    static std::uint8_t quadrant(void* c) {
        auto& s=self(c);return s.services.damage.quadrant(s.services.damage.context);
    }
    static bool displayWindow(void* c) {
        auto& s=self(c);return s.services.damage.displayWindow(s.services.damage.context);
    }
    static void hitCallback(void* c,std::uint32_t ea,s_workAreaCopy* owner,sBattleTargetable& target) {
        auto& s=self(c);s.services.damage.hitCallback(s.services.damage.context,ea,owner,target);
    }
    static void hitDisplay(void* c,s_workAreaCopy* owner,sBattleTargetable& target,int mode) {
        auto& s=self(c);s.services.damage.hitDisplay(s.services.damage.context,owner,target,mode);
    }
    static void damageDisplay(void* c,s_workAreaCopy* owner,std::int16_t damage,sVec3_FP* position,int mode) {
        auto& s=self(c);s.services.damage.damageDisplay(s.services.damage.context,owner,damage,position,mode);
    }
    static std::uint32_t random(void* c) {
        auto& s=self(c);return s.services.random(s.services.damage.context);
    }
    static void sound(void* c,int id) {
        auto& s=self(c);s.services.sound(s.services.damage.context,id);
    }
    static void recordDeath(void* c) {
        auto& s=self(c);s.services.recordDeath(s.services.damage.context);
    }
    static void hideDrawing(void* c) {self(c).m_DrawMethod=nullptr;}
    static void Update(sLagiTutorialLaserEnemyTask* s) {
        const LagiTutorialLaserEnemyHooks hooks{{s,quadrant,displayWindow,hitCallback,hitDisplay,damageDisplay},
            random,sound,recordDeath,hideDrawing};
        lagiUpdateTutorialLaserEnemy(s->view(),hooks);
    }
    static void Draw(sLagiTutorialLaserEnemyTask* s) {
        lagiDrawBattleEnemyModels(s->flags,s->lightColor,s->position,s->rotation,s->models);
    }
};
s_workAreaCopy* lagiCreateTutorialLaserEnemyTask(const LagiTutorialLaserEnemySpawn& spawn)
{
    const auto& h=spawn.services;
    if (!spawn.parent || !spawn.resources || !spawn.resources->m0_fileBundle ||
        !h.damage.quadrant || !h.damage.displayWindow || !h.damage.hitCallback ||
        !h.damage.hitDisplay || !h.damage.damageDisplay || !h.random || !h.sound || !h.recordDeath)
        return nullptr; // Incomplete binding must not create a pretend enemy.
    static const sLagiTutorialLaserEnemyTask::TypedTaskDefinition definition={
        nullptr,sLagiTutorialLaserEnemyTask::Update,sLagiTutorialLaserEnemyTask::Draw,nullptr};
    auto* s=createSubTaskWithCopy<sLagiTutorialLaserEnemyTask>(spawn.parent,&definition);
    if (!s) return nullptr;
    s->formationPosition=&spawn.formationPosition;s->formationRotation=&spawn.formationRotation;
    s->deadCount=&spawn.deadCount;s->experience=&spawn.experience;s->dyne=&spawn.dyne;
    s->authoredOffset=spawn.authoredOffset;s->resources=spawn.resources;s->services=spawn.services;
    const LagiTutorialLaserEnemyInitView initial{s,spawn.resources->m0_fileBundle,s->models,
        s->modelCount,s->targets,s->lifeMeter,s->health,s->entityIndex,s->motion(),s->rate};
    if (!lagiInitTutorialLaserEnemy(initial,spawn.entityIndex,spawn.authoredOffset,
        spawn.modelDescriptors,spawn.callbacks,sLagiTutorialLaserEnemyTask::random,s))
        return nullptr; // Initializer finishes the failed task/life meter.
    return s;
}
