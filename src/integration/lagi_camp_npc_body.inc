// CAMP0606ABE8 / definition0607EB94. Native34-byte task, adapted to typed
// parent poses and the existing Azel segmented-body services. No raw host offsets.
#include "kernel/monsterPart.h"
#include "lagi/lagi_render_bridge.h"
#include <bit>

void transformVec(const sVec3_FP&,sVec3_FP&,const sMatrix4x3&);

struct sLagiCampNpcBody : s_workAreaTemplateWithArgWithCopy<sLagiCampNpcBody,sNPC*>
{
    sNPC* parent{};
    sVec3_FP position,rotation,previousPosition;
    sMonsterBody* body{};

    // CAMP0606A888. Same attachment hierarchy for normal and interpolated
    // poses; only their native strides differ (B4 and48 respectively).
    template<class Poses> void CaptureAttachment(const Poses& poses)
    {
        sMatrix4x3 matrix;
        initMatrixToIdentity(&matrix);
        translateMatrix(parent->mE8.m0_position,&matrix);
        rotateMatrixShiftedY(parent->mE8.mC_rotation[1],&matrix);
        rotateMatrixShiftedX(parent->mE8.mC_rotation[0],&matrix);
        rotateMatrixShiftedY(0x08000000,&matrix);
        translateMatrix(poses[0].m0_translation,&matrix);
        rotateMatrixZYX(&poses[0].mC_rotation,&matrix);
        translateMatrix(poses[1].m0_translation,&matrix);
        rotateMatrixShiftedZ(poses[1].mC_rotation[2],&matrix);
        rotateMatrixShiftedY(poses[1].mC_rotation[1]+MTH_Mul(parent->m20_lookAtAngle[1],0x4CCC),&matrix);
        rotateMatrixShiftedX(poses[1].mC_rotation[0],&matrix);
        translateMatrix(poses[2].m0_translation,&matrix);
        rotateMatrixShiftedZ(poses[2].mC_rotation[2],&matrix);
        rotateMatrixShiftedY(poses[2].mC_rotation[1]+MTH_Mul(parent->m20_lookAtAngle[1],0xB333),&matrix);
        rotateMatrixShiftedX(poses[2].mC_rotation[0]+parent->m20_lookAtAngle[0],&matrix);
        translateMatrix(readSaturnVec3(gCurrentTownOverlay->getSaturnPtr(0x0607EB88)),&matrix);
        rotateMatrixShiftedY(static_cast<s32>(0xFE5843F2u),&matrix);
        previousPosition=position;
        position=matrix.getTranslation();
        // MAIN0603647C; use the same original trig services and matrix terms.
        rotation[0]=atan_FP(-matrix.m[1][2]);
        rotation[1]=atan2_FP(matrix.m[0][2],matrix.m[2][2]);
        rotation[2]=atan2_FP(matrix.m[1][0],matrix.m[1][1]);
    }

    void CaptureAttachment()
    {
        const auto& model=parent->m34_3dModel;
        if(!model.m48_poseDataInterpolation.empty())CaptureAttachment(model.m48_poseDataInterpolation);
        else CaptureAttachment(model.m2C_poseData);
    }

    // CAMP0606AAB8. Inverse Z/X/Y rotation converts the world displacement
    // before the native SHLL14 impulses are written to the first linked part.
    void ApplyMotionImpulse()
    {
        sMatrix4x3 inverse;
        initMatrixToIdentity(&inverse);
        rotateMatrixShiftedZ(-rotation[2],&inverse);
        rotateMatrixShiftedX(-rotation[0],&inverse);
        rotateMatrixShiftedY(-rotation[1],&inverse);
        sVec3_FP local;
        transformVec(position-previousPosition,local,inverse);
        auto shift14=[](fixedPoint value){return std::bit_cast<s32>(u32(value.asS32())<<14);};
        auto& acceleration=body->m30_parts[0].m34_rotationAcceleration;
        acceleration[0]=shift14(local[2]);
        acceleration[1]=0;
        acceleration[2]=shift14(local[0]);
    }

    static void Init(sLagiCampNpcBody* task,sNPC* parent)
    {
        task->parent=parent;
        task->position.zeroize();task->rotation.zeroize();task->previousPosition.zeroize();
        // CAMP0606AA04 initializes temporary model8 with pose70. Its first7
        // translations seed the links; preserve the actual model initializer.
        s_3dModel seedModel;
        auto* bundle=task->m0_fileBundle;
        auto* hierarchy=bundle->getModelHierarchy(8);
        auto* pose=bundle->getStaticPose(0x70,hierarchy->countNumberOfBones());
        init3DModelRawData(task,&seedModel,0,bundle,8,nullptr,pose,nullptr,nullptr);
        task->body=monsterBody_create(task,7);
        monsterBody_loadPartData(task->body,1,gCurrentTownOverlay->getSaturnPtr(0x0607EAFC));
        for(unsigned i=0;i<7;++i)
            task->body->m30_parts[i].m10_translation=seedModel.m2C_poseData[i].m0_translation;
        task->CaptureAttachment();
        monsterBody_initPose(task->body,&task->position,&task->rotation);
    }

    static void Update(sLagiCampNpcBody* task)
    {
        task->CaptureAttachment();task->ApplyMotionImpulse();
        task->body->m0_translation=task->position;
        // Native writes+18, not+0C: retain the initial base rotation.
        task->body->m18_rotationTarget=task->rotation;
        task->body->m24_update(&task->body->m30_parts[0],&task->body->m0_translation,
            &task->body->mC_rotation,&task->body->m18_rotationTarget);
    }

    static void Draw(sLagiCampNpcBody* task)
    {
        lagi::azel_bridge::begin_view_relative_submission_scope();
        task->body->m28_draw(task->m0_fileBundle,&task->body->m30_parts[0]);
        lagi::azel_bridge::end_view_relative_submission_scope();
    }

    static void Delete(sLagiCampNpcBody* task)
    {
        // Native task heaps own both allocations; the shared host service uses
        // a C++ vector, so its owner must explicitly release it at task teardown.
        delete task->body;task->body=nullptr;
    }
    static TypedTaskDefinition* getTypedTaskDefinition()
    {
        static TypedTaskDefinition definition={&Init,&Update,&Draw,&Delete};
        return &definition;
    }
};

static sLagiCampNpcBody* lagiCreateCampNpcBody(sNPC* parent)
{
    return createSubTaskWithArgWithCopy<sLagiCampNpcBody,sNPC*>(parent,parent);
}
