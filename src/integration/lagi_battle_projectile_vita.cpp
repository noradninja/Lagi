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
