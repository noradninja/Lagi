#include "lagi/lagi_azel_upstream_prelude.h"
#include "battle/battleManager.h"
#include "battle/battleOverlay.h"
#include "battle/battleEngine.h"
#include "battle/battleDragon.h"
#include "battle/battleTargetable.h"
#include "battle/BTL_A3/BTL_A3.h"
#include "battle/BTL_A3/BTL_A3_data.h"
#include "battle/particleEffect.h"
#include "kernel/fileBundle.h"
#include "kernel/animation.h"
#include "kernel/vdp1Allocator.h"
#include "kernel/graphicalObject.h"
#include "audio/systemSounds.h"
#include "mainMenuDebugTasks.h"
#include <map>
#include "lagi/native_battle_projectiles.h"
#include "lagi_battle_projectile_descriptor.inc"
#include "lagi_battle_projectile_motion.inc"

// Content-keyed immutable runtime descriptor cache. Particle tasks can outlive
// their projectile and overlay; neither borrowed Saturn pointers nor pointers
// into a projectile-owned vector are safe keys/lifetimes here. Only authored
// quad metadata is copied, never native model ownership or texture resources.
static const std::vector<sVdp1Quad>* lagiProjectileQuad(sSaturnPtr ptr)
{
    static std::map<std::vector<u8>,std::vector<sVdp1Quad>> cache;
    auto quads=initVdp1Quad(ptr);
    std::vector<u8> key;
    key.reserve(quads.size()*0x1C);
    for(size_t i=0;i<quads.size()*0x1C;++i)key.push_back(readSaturnU8(ptr+u32(i)));
    const auto found=cache.find(key);
    if(found!=cache.end())return &found->second;
    return &cache.emplace(std::move(key),std::move(quads)).first->second;
}

// Shared 06064400 boundary used by native shots and actor rush attacks.
// Keep the geometry service independent of each caller's damage protocol.
bool lagiBattleContactPoint(const sVec3_FP& source,std::int32_t sourceRadius,
    const sVec3_FP& target,std::int32_t targetRadius,sVec3_FP& worldContact)
{
    sLagiBattleSphere sourceView{},targetView{};
    transformAndAddVecByCurrentMatrix(&source,&sourceView.position);
    transformAndAddVecByCurrentMatrix(&target,&targetView.position);
    sourceView.radius=sourceRadius;targetView.radius=targetRadius;
    return lagiBattleSphereImpact(sourceView,targetView,worldContact);
}

struct sLagiBattleProjectile: s_workAreaTemplateWithCopy<sLagiBattleProjectile>
{
    sVec3_FP position{},velocity{},acceleration{};
    s_3dModel* model{};
    sAnimatedQuad quad{};
    s16 timer{},damage{};
    u8 damageType{},hitSound=0xFF;
    s16 trailMask{};
    u8 trailSlot{},hitSlot{};
    const std::vector<sVdp1Quad>* trail{};
    const std::vector<sVdp1Quad>* hit{};
};

static void lagiProjectileParticle(u8 slot,const std::vector<sVdp1Quad>* quad,
    const sVec3_FP& position)
{
    if(quad)createParticleEffect(dramAllocatorEnd[slot].mC_fileBundle,quad,
        &position,nullptr,nullptr,0x10000,nullptr,0);
}

static void lagiBattleProjectileUpdate(sLagiBattleProjectile* task)
{
    // 060A44DE..060A4688, including the two separate impact/expiry emissions.
    task->position+=task->velocity;
    task->velocity+=task->acceleration;
    if(task->model)stepAnimation(task->model);
    else sGunShotTask_UpdateSub4(&task->quad);
    if(task->trail && !(s32(task->trailMask)&randomNumber()))
        lagiProjectileParticle(task->trailSlot,task->trail,task->position);
    auto* dragon=gBattleManager->m10_battleOverlay->m18_dragon;
    sLagiBattleSphere projectileView{},dragonView{};
    transformAndAddVecByCurrentMatrix(&task->position,&projectileView.position);
    transformAndAddVecByCurrentMatrix(&dragon->m8_position,&dragonView.position);
    dragonView.radius=0x5000;
    if(lagiBattleSpheresOverlap(projectileView,dragonView)){
        applyDamageToDragon(dragon->m8C,task->damage,dragon->m8_position,3,
            task->velocity,task->damageType);
        lagiProjectileParticle(task->hitSlot,task->hit,task->position);
        task->timer=0;
    }
    if(--task->timer<0){
        lagiProjectileParticle(task->hitSlot,task->hit,task->position);
        if(s8(task->hitSound)>=0)playSystemSoundEffect(s8(task->hitSound));
        task->getTask()->markFinished();
    }
}

static void lagiBattleProjectileDraw(sLagiBattleProjectile* task)
{
    if(task->timer<0)return;
    if(!task->model){drawProjectedParticle(&task->quad,&task->position);return;}
    sVec2_FP angles;computeLookAt(task->velocity,angles);
    sVec3_FP rotation;
    rotation[0]=-angles[0];rotation[1]=angles[1]+fixedPoint(0x08000000);rotation[2]=0;
    pushCurrentMatrix();translateCurrentMatrix(&task->position);
    rotateCurrentMatrixYXZ(&rotation);
    task->model->m18_drawFunction(task->model);
    popMatrix();
}

static void lagiBattleProjectileDelete(sLagiBattleProjectile* task)
{
    // Parsed bundle geometry stays file-owned. Only the pose/matrix/animation
    // work area was allocated here. Native work areas lack C++ destructors.
    delete task->model;task->model=nullptr;
}

// Recovered 060A486C typed service, shared by the authored urchin attack routes.
p_workArea lagiCreateBattleProjectile(const sVec3_FP& source,const sVec3_FP& target,
    const sLagiBattleProjectileDescriptor& d)
{
    static const sLagiBattleProjectile::TypedTaskDefinition definition={
        nullptr,lagiBattleProjectileUpdate,lagiBattleProjectileDraw,lagiBattleProjectileDelete};
    auto* owner=dramAllocatorEnd[d.slot].mC_fileBundle;
    auto* task=createSubTaskWithCopy<sLagiBattleProjectile>(
        gBattleManager->m10_battleOverlay->m18_dragon,&definition);
    if(!task)return nullptr;
    task->m0_fileBundle=owner->m0_fileBundle;
    task->m4_vd1Allocation=owner->m4_vd1Allocation;
    if(d.hierarchy){
        task->model=new s_3dModel();
        auto* bundle=owner->m0_fileBundle;
        auto* hierarchy=bundle->getModelHierarchy(d.hierarchy);
        if(!hierarchy || !init3DModelRawData(task,task->model,0,bundle,d.hierarchy,
            d.animation?bundle->getAnimation(d.animation):nullptr,
            bundle->getStaticPose(d.pose,hierarchy->countNumberOfBones()),nullptr,nullptr)){
            task->m_UpdateMethod=nullptr;task->m_DrawMethod=nullptr;
            task->getTask()->markFinished();return task;
        }
    }else{
        particleInitSub(&task->quad,owner->m4_vd1Allocation->m4_vdp1Memory,
            lagiProjectileQuad(d.quad));
        const unsigned advance=randomNumber()&0xF;
        for(unsigned i=0;i<advance;++i)sGunShotTask_UpdateSub4(&task->quad);
    }
    task->position=source;
    lagiBattleProjectileMotion(source,target,d,
        gBattleManager->m10_battleOverlay->m4_battleEngine->m1A0_battleAutoScrollDelta,
        task->velocity,task->acceleration);
    task->timer=d.lifetime;task->damage=d.damage;task->damageType=d.damageType;
    if(!d.muzzle.isNull())lagiProjectileParticle(d.muzzleSlot,lagiProjectileQuad(d.muzzle),source);
    if(!d.trail.isNull()){
        task->trail=lagiProjectileQuad(d.trail);
        task->trailSlot=d.trailSlot;task->trailMask=d.trailMask;
    }
    if(!d.hit.isNull()){
        task->hit=lagiProjectileQuad(d.hit);task->hitSlot=d.hitSlot;
    }
    return task;
}

// Native homing shots use the same immutable quad cache, task ownership,
// projected-particle rendering and dragon damage service as other projectiles.
// Their motion/timing/impact protocol is distinct from the ballistic route.
struct sLagiHomingBattleProjectile: s_workAreaTemplateWithCopy<sLagiHomingBattleProjectile>
{
    sVec3_FP position{},velocity{},acceleration{};
    sAnimatedQuad quad{};
    s16 delay{},timer{},damage{};
    u8 damageType{};
    bool impacted{};
};

static void lagiHomingBattleProjectileUpdate(sLagiHomingBattleProjectile* task)
{
    if(task->impacted)return;
    auto* overlay=gBattleManager->m10_battleOverlay;
    task->position+=overlay->m4_battleEngine->m1A0_battleAutoScrollDelta;
    sGunShotTask_UpdateSub4(&task->quad);
    if(task->delay-->0)return;
    auto* dragon=overlay->m18_dragon;
    lagiBattleHomingProjectileMotion(dragon->m8_position,task->position,
        task->velocity,task->acceleration);
    sVec3_FP impact;
    if(lagiBattleContactPoint(task->position,0x2000,dragon->m8_position,0x1000,impact)){
        const sVec3_FP zero{};
        applyDamageToDragon(dragon->m8C,task->damage,impact,2,zero,task->damageType);
        task->impacted=true;
        task->m_DrawMethod=nullptr;
        task->getTask()->markFinished();
    }
    if(task->timer--<0){
        task->m_DrawMethod=nullptr;
        task->getTask()->markFinished();
    }
}

static void lagiHomingBattleProjectileDraw(sLagiHomingBattleProjectile* task)
{
    drawProjectedParticle(&task->quad,&task->position);
}

// 06054080. The caller supplies authored resource/quad/damage data; the common
// service preserves the original delay, lifetime, homing and one-hit protocol.
s_workArea* lagiCreateNativeHomingBattleProjectile(s_workAreaCopy* parent,
    const sVec3_FP& source,const sVec3_FP& velocity,sSaturnPtr quad,
    std::uint8_t slot,std::int16_t damage,std::uint8_t damageType)
{
    static const sLagiHomingBattleProjectile::TypedTaskDefinition definition={
        nullptr,lagiHomingBattleProjectileUpdate,lagiHomingBattleProjectileDraw,nullptr};
    auto* task=createSubTaskWithCopy<sLagiHomingBattleProjectile>(parent,&definition);
    if(!task)return nullptr;
    auto* owner=dramAllocatorEnd[slot].mC_fileBundle;
    task->m0_fileBundle=owner->m0_fileBundle;
    task->m4_vd1Allocation=owner->m4_vd1Allocation;
    task->position=source;task->velocity=velocity;
    task->delay=randomNumber()&9;
    task->timer=0x4B;
    task->damage=damage;task->damageType=damageType;
    particleInitSub(&task->quad,owner->m4_vd1Allocation->m4_vdp1Memory,
        lagiProjectileQuad(quad));
    return task;
}

struct sLagiBattleChargeQuad: s_workAreaTemplateWithCopy<sLagiBattleChargeQuad>
{
    const sVec3_FP* source{};
    const std::uint8_t* state{};
    std::uint8_t activeState{};
    sVec3_FP position{};
    sAnimatedQuad quad{};
    std::vector<sVdp1Quad>* frames{};
    fixedPoint width{},height{};
};

// Native 0605C582. State is checked before reading the borrowed hotpoint:
// actor retirement may stop updating it, but storage stays parent-task-owned.
static void lagiBattleChargeQuadUpdate(sLagiBattleChargeQuad* task)
{
    if(*task->state!=task->activeState){
        task->getTask()->markFinished();
        return;
    }
    task->position=*task->source;
    task->position[1]+=fixedPoint(0x2000);
    task->position[2]+=fixedPoint(0x2000);
    sGunShotTask_UpdateSub4(&task->quad);
    task->width+=fixedPoint(0x333);
    task->height+=fixedPoint(0x333);
    if(task->width.m_value>=0x4000)task->width=0x4000;
    if(task->height.m_value>=0x4000)task->height=0x4000;
}

static void lagiBattleChargeQuadDraw(sLagiBattleChargeQuad* task)
{
    auto& frame=(*task->frames)[task->quad.m7_currentFrame];
    frame.mC_width=task->width;
    frame.m10_height=task->height;
    drawProjectedParticle(&task->quad,&task->position);
}

static void lagiBattleChargeQuadDelete(sLagiBattleChargeQuad* task)
{
    delete task->frames;task->frames=nullptr;
}

// Native 0605C51C. The scheduler retires children with their parent. Its state
// and targetable hotpoint therefore outlive this task; no host layout offsets.
s_workArea* lagiCreateNativeBattleChargeQuad(s_workAreaCopy* parent,
    const sVec3_FP* position,const std::uint8_t* state,std::uint8_t activeState,
    sSaturnPtr quad,std::uint8_t slot)
{
    static const sLagiBattleChargeQuad::TypedTaskDefinition definition={
        nullptr,lagiBattleChargeQuadUpdate,lagiBattleChargeQuadDraw,lagiBattleChargeQuadDelete};
    auto* task=createSubTaskWithCopy<sLagiBattleChargeQuad>(parent,&definition);
    if(!task)return nullptr;
    auto* owner=dramAllocatorEnd[slot].mC_fileBundle;
    task->m0_fileBundle=owner->m0_fileBundle;
    task->m4_vd1Allocation=owner->m4_vd1Allocation;
    task->source=position;task->position=*position;
    task->state=state;task->activeState=activeState;
    task->frames=new std::vector<sVdp1Quad>(*lagiProjectileQuad(quad));
    particleInitSub(&task->quad,owner->m4_vd1Allocation->m4_vdp1Memory,task->frames);
    return task;
}
