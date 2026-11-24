#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/garil_story_worm.h"
#include "lagi/native_task_disposal.h"
#include "field/field_a5/o_fld_a5.h"
#include "field/field_a5/a5_gridDeferredDraw.h"
#include "field/fieldCutsceneTask.h"
#include "field/fieldCutsceneTask2.h"
#include "kernel/monsterPart.h"
#include "kernel/fileBundle.h"
#include "kernel/animation.h"
#include "audio/systemSounds.h"
#include <map>

// Original task descriptor 0609D7BC, NOT the adjacent 0609D7CC actor.
struct sLagiA5StoryWorm:s_workAreaTemplateWithArg<sLagiA5StoryWorm,sSaturnPtr> {
    s_memoryAreaOutput memory;
    sVec3_FP position,cameraAnchor,rotation,initialPosition,initialRotation;
    sVec3_FP velocity;
    std::shared_ptr<sMonsterBody> body;
    s_3dModel head,tail;
    std::vector<s32> oldPartY;
    std::vector<u8> partEffects;
    int type=0,partCount=0,speed=0x1AAA;
    fixedPoint stiffness=0xE00,damping=0x5000;
    static void Init(sLagiA5StoryWorm* self,sSaturnPtr data) {
        getMemoryArea(&self->memory,1);
        self->type=readSaturnS16(data);self->partCount=readSaturnS16(data+2);
        assert(self->type>=0&&self->type<3&&self->partCount>=2);
        self->position=self->initialPosition=readSaturnVec3(data+4);
        self->rotation=self->initialRotation=readSaturnVec3(data+16);
        self->oldPartY.resize(self->partCount);self->partEffects.resize(self->partCount);
        self->body.reset(monsterBody_create(self,self->partCount));
        monsterBody_loadPartData(self->body.get(),0,sSaturnPtr());
        auto table=[&](u32 ea){return readSaturnU32(gFLD_A5->getSaturnPtr(ea+self->type*4));};
        for(int i=0;i<self->partCount;++i) {
            auto& part=self->body->m30_parts[i];
            part.m10_translation=readSaturnVec3(gFLD_A5->getSaturnPtr(i==0?0x0609D7B0:0x0609D78C+self->type*12));
            monsterBodyPart_loadTemplate(&part,gFLD_A5->getSaturnPtr(table(i==0?0x0609D720:i==self->partCount-1?0x0609D738:0x0609D72C)));
            part.m44_springStiffness={self->stiffness,self->stiffness,self->stiffness};
            part.m50_damping=self->damping;
        }
        monsterBody_initPose(self->body.get(),&self->position,&self->rotation);
        self->cameraAnchor={self->position.m0_X,0,self->position.m8_Z};
        auto* bundle=self->memory.m0_mainMemoryBundle;
        auto model=[&](s_3dModel& out,u32 modelTable,u32 poseTable,u32 animationTable) {
            auto* anim=bundle->getAnimation(table(animationTable));
            auto* pose=bundle->getStaticPose(table(poseTable),anim->m2_numBones);
            const auto result=init3DModelRawData(self,&out,0,bundle,table(modelTable),anim,pose,nullptr,nullptr);
            if(result) stepAnimation(&out);
            return result!=0;
        };
        if(!model(self->head,0x0609D744,0x0609D75C,0x0609D774)||
           !model(self->tail,0x0609D750,0x0609D768,0x0609D780))self->getTask()->markFinished();
    }
    static void Update(sLagiA5StoryWorm* self) {
        stepAnimation(&self->head);stepAnimation(&self->tail);
        // Native 060662DE: signed 28-bit angles, 15/16 with truncation to zero.
        self->rotation.m0_X=self->rotation.m0_X.normalized().asS32()*15/16;
        self->rotation.m8_Z=self->rotation.m8_Z.normalized().asS32()*15/16;
        const auto x=(self->rotation.m0_X.asS32()>>16)&0xFFF;
        const auto y=(self->rotation.m4_Y.asS32()>>16)&0xFFF;
        self->velocity.m0_X=MTH_Mul_5_6(self->speed,getCos(x),getSin(y));
        self->velocity.m4_Y=-MTH_Mul(self->speed,getSin(x));
        self->velocity.m8_Z=MTH_Mul_5_6(self->speed,getCos(x),getCos(y));
        const auto oldY=self->position.m4_Y;
        self->position+=self->velocity;
        if(MTH_Mul(oldY,self->position.m4_Y)<=0) {
            if(isWorldPositionOnScreen(&self->position))playSystemSoundEffect(0x6E);
            lagiSpawnA5WormBreachSand(&self->position);
        }
        auto& b=*self->body;
        for(int i=0;i<self->partCount;++i)self->oldPartY[i]=b.m30_parts[i].m4_worldPosition.m4_Y.asS32();
        self->cameraAnchor={self->position.m0_X,0,self->position.m8_Z};
        b.m0_translation=self->position;b.mC_rotation=self->rotation;b.m18_rotationTarget=self->rotation;
        b.m24_update(&b.m30_parts[0],&b.m0_translation,&b.mC_rotation,&b.m18_rotationTarget);
        // Native m110 is zero-initialized and is not written by this actor:
        // its half-negative radius threshold is therefore zero.
        for(int i=0;i<self->partCount;++i) {
            auto& p=b.m30_parts[i].m4_worldPosition;
            if(MTH_Mul(p.m4_Y,fixedPoint(self->oldPartY[i]))<=0) {
                lagiSpawnA5WormSegmentSand(&p);self->partEffects[i]=0;
            }
            if((i&1)&&p.m4_Y<0&&self->partEffects[i]==0) {
                lagiSpawnA5WormSegmentDebris(self->body,i);self->partEffects[i]=1;
            }
        }
    }
    static void Draw(sLagiA5StoryWorm* self) {
        auto* bundle=self->memory.m0_mainMemoryBundle;auto& parts=self->body->m30_parts;
        auto animated=[&](sMonsterBodyPart& part,s_3dModel& model,int collision) {
            if(part.m4_worldPosition.m4_Y<0)return;
            pushCurrentMatrix();translateCurrentMatrix(&part.m4_worldPosition);
            rotateCurrentMatrixYXZ(&part.m1C_rotation);model.m18_drawFunction(&model);
            callGridCellDraw_normalSub2(bundle,collision);popMatrix();
        };
        animated(parts.front(),self->head,0x274);
        for(int i=1;i<self->partCount-1;++i)if(parts[i].m4_worldPosition.m4_Y>=0) {
            self->body->m2C_drawPart(bundle,&parts[i]);
            a5GridDraw_cullAndEnqueueWithTransform_06077074(bundle,0x144,&parts[i].m4_worldPosition,&parts[i].m1C_rotation);
        }
        animated(parts.back(),self->tail,0x27C);
        for(auto& part:parts)if(part.m4_worldPosition.m4_Y>0) {
            sVec3_FP floor={part.m4_worldPosition.m0_X,0,part.m4_worldPosition.m8_Z};
            pushCurrentMatrix();translateCurrentMatrix(&floor);rotateCurrentMatrixShiftedX(0x4000000);
            addObjectToDrawList(bundle->get3DModel(0xB0));popMatrix();
        }
    }
    static void Delete(sLagiA5StoryWorm* self){self->body.reset();}
};
s_workArea* lagiCreateA5StoryWorm(s_workArea* parent,sSaturnPtr data) {
    static const sLagiA5StoryWorm::TypedTaskDefinition def={sLagiA5StoryWorm::Init,sLagiA5StoryWorm::Update,sLagiA5StoryWorm::Draw,sLagiA5StoryWorm::Delete};
    auto* actor=createSubTaskWithArg<sLagiA5StoryWorm>(parent,data,&def);
    if(actor)lagi::tasks::register_work_area_disposer(actor,[](void* p){delete static_cast<sLagiA5StoryWorm*>(p);});
    return actor&&!actor->getTask()->isFinished()?actor:nullptr;
}
sVec3_FP* lagiA5StoryWormCameraAnchor(s_workArea* actor) {
    return &static_cast<sLagiA5StoryWorm*>(actor)->cameraAnchor;
}
struct sLagiA5WormTracker:s_workAreaTemplate<sLagiA5WormTracker> {
    sLagiA5StoryWorm* actor; sSaturnPtr commands;int threshold,bit,state=0;unsigned frame=0;
    static void Update(sLagiA5WormTracker* self) {
        auto& a=*self->actor;
        if(self->state==0)self->state=1;
        // Native actor+0x10 is POSITION Z, despite the upstream Y comment.
        if(self->state==1&&a.position.m8_Z>=self->threshold) {mainGameState.setBit566(self->bit);self->state=2;}
        getFieldTaskPtr()->m28_status|=0x8000;
        const s16 command=readSaturnS16(self->commands);
        if(command<0) {
            a.getTask()->markFinished();mainGameState.setBit566(self->bit);
            getFieldTaskPtr()->m28_status&=~0x8000u;self->getTask()->markFinished();return;
        }
        if(command&1)a.rotation.m0_X+=0x888888;
        if(command&2)a.rotation.m0_X-=0x888888;
        if(command&8)a.rotation.m4_Y-=0x888888;
        if(command&4)a.rotation.m4_Y+=0x888888;
        if(command&0x800)a.rotation.m8_Z+=0x888888;
        if(command&0x400)a.rotation.m8_Z-=0x888888;
        if(command&0x10)a.speed+=0x111;
        if(command&0x20)a.speed=std::max(0,a.speed-0x111);
        self->commands+=2;
        if(self->frame==0||self->frame==30||self->frame==45)playSystemSoundEffect(0x6C);
        ++self->frame;
    }
};
void lagiTrackA5StoryWorm(s_workArea* actor,sSaturnPtr commands,int threshold,int bit) {
    if(!actor)return;
    auto* tracker=createSubTaskFromFunction<sLagiA5WormTracker>(actor,&sLagiA5WormTracker::Update);
    tracker->actor=static_cast<sLagiA5StoryWorm*>(actor);tracker->commands=commands;
    tracker->threshold=threshold;tracker->bit=bit;
}
void lagiStartA5StoryWormCamera(s_workArea* parent,sSaturnPtr script,unsigned callback,sVec3_FP* subject) {
    // Stable per-script vectors: shared camera tasks retain their vector address.
    static std::map<u32,std::vector<s_scriptData1>> scripts;
    auto& data=scripts[script.m_offset];loadScriptData1(data,script);
    cutsceneTaskInitSub2(parent,data,callback,subject,0);
}
