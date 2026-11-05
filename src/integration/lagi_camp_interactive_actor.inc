#include "lagi/lagi_render_bridge.h"
#include "3dEngine.h"
// Disc 1 TWN_CAMP definition0607B5BC. Offsets below name Saturn fields;
// host objects use typed members and never assume Saturn pointer/layout sizes.
struct sLagiCampInteractiveActor : s_workAreaTemplateWithArgAndBase<sLagiCampInteractiveActor, sTownObject, sSaturnPtr>
{
    static TypedTaskDefinition* getTypedTaskDefinition()
    {
        static TypedTaskDefinition definition = {&Init, &AwaitResource, nullptr, &Delete};
        return &definition;
    }

    static void Init(sLagiCampInteractiveActor* self, sSaturnPtr arg)
    {
        self->mC_argument = arg;
        self->m10_state = 0;
        self->m14_sound = self->m15_soundActive = self->m16_variant = 0;
        self->m18_paletteSequence = sSaturnPtr::getNull();
        self->m1C_position = self->mE8_position = readSaturnVec3(arg + 8);
        self->m28_rotation = self->mF4_rotation = readSaturnVec3(arg + 0x14);
        self->mE8_position[1] += 0x1333;
        npcData0.m70_npcPointerArray[readSaturnS32(arg + 0x20)].workArea = self;
    }

    static void initModel(sLagiCampInteractiveActor* self, s_3dModel* model,
                          u16 hierarchy, u16 pose, u16 animation)
    {
        auto* bundle = self->m0_fileBundle;
        auto* staticPose = bundle->getStaticPose(pose,
            bundle->getModelHierarchy(hierarchy)->countNumberOfBones());
        init3DModelRawData(self, model, 0, bundle, hierarchy,
                         bundle->getAnimation(animation), staticPose, nullptr, nullptr);
    }

    static void initVariant(sLagiCampInteractiveActor* self, s32 offset)
    {
        const auto descriptor = gCurrentTownOverlay->getSaturnPtr(0x0607B308) + offset;
        // Native06020024 releases the old model's allocations before replacement.
        self->m100_model = {};
        initModel(self, &self->m100_model, readSaturnU16(descriptor),
                  readSaturnU16(descriptor + 2), readSaturnU16(descriptor + 4));
    }

    static void AwaitResource(sLagiCampInteractiveActor* self)
    {
        if (!isDataLoaded(readSaturnS32(self->mC_argument))) return;
        initModel(self, &self->m34_model, 0x24, 0x458, 0x480);
        updateAndInterpolateAnimation(&self->m34_model);
        initVariant(self, 0);
        const auto bounds = gCurrentTownOverlay->getSaturnPtr(0x0607B2D8);
        setupBody(self, &self->m84_body, &self->m1C_position, &self->m28_rotation,
                  readSaturnEA(self->mC_argument + 0x24), bounds);
        setupBody(self, &self->m150_body, &self->mE8_position, &self->mF4_rotation,
                  readSaturnEA(self->mC_argument + 0x28), bounds + 0x18);
        self->m_UpdateMethod = &Update;
        self->m_DrawMethod = &Draw;
    }

    static void setupBody(sLagiCampInteractiveActor* self, sCollisionBody* body,
                          sVec3_FP* position, sVec3_FP* rotation, sSaturnPtr script,
                          sSaturnPtr bounds)
    {
        body->m30_pPosition = position;
        body->m34_pRotation = rotation;
        body->m38_pOwner = self;
        body->m3C_scriptEA = script;
        body->m40 = nullptr;
        setCollisionSetup(body, 4);
        setCollisionBounds(body, readSaturnVec3(bounds), readSaturnVec3(bounds + 0xC));
    }

    // Native06057766 consumes a palette sequence, not an animation sequence.
    static void updatePalette(sLagiCampInteractiveActor* self)
    {
        if (self->m18_paletteSequence.isNull()) return;
        auto palette = readSaturnEA(self->m18_paletteSequence);
        self->m18_paletteSequence += 4;
        if (palette.isNull())
        {
            const s32 displacement = readSaturnS32(self->m18_paletteSequence);
            if (!displacement)
            {
                self->m18_paletteSequence = sSaturnPtr::getNull();
                return;
            }
            self->m18_paletteSequence += (displacement - 1) * 4;
            palette = readSaturnEA(self->m18_paletteSequence);
            self->m18_paletteSequence += 4;
        }
        asyncDmaCopy(palette, getVdp2Cram(0x240), 0x20, 0);
        self->mF4_rotation[1] -= 0x5B05B;
        updateAndInterpolateAnimation(&self->m100_model);
        registerCollisionBody(&self->m150_body);
    }

    static bool primaryAnimationFinished(sLagiCampInteractiveActor* self)
    {
        const s16 frame = updateAndInterpolateAnimation(&self->m34_model);
        const s32 frames = self->m34_model.m30_pCurrentAnimation
            ? static_cast<s16>(self->m34_model.m30_pCurrentAnimation->m4_numFrames) : 0;
        return frame >= frames - 1;
    }

    static void Update(sLagiCampInteractiveActor* self)
    {
        registerCollisionBody(&self->m84_body);
        updatePalette(self);
        switch (self->m10_state)
        {
        case 1:
            initVariant(self, static_cast<s8>(self->m16_variant * 6));
            self->m16_variant = static_cast<s8>(self->m16_variant + 1);
            if (self->m16_variant > 6) self->m16_variant = 0;
            initAnimation(&self->m34_model, self->m0_fileBundle->getAnimation(0x480));
            self->m10_state = 2;
            [[fallthrough]];
        case 2:
            if (primaryAnimationFinished(self))
            {
                playPCM(self, self->m14_sound + 0x65);
                self->m14_sound ^= 1;
                self->m15_soundActive = 1;
                self->m18_paletteSequence = gCurrentTownOverlay->getSaturnPtr(0x0607B534);
                self->m10_state = 3;
            }
            break;
        case 4:
            initAnimation(&self->m34_model, self->m0_fileBundle->getAnimation(0x484));
            playPCM(self, 0x64);
            self->m15_soundActive = 0;
            self->m18_paletteSequence = gCurrentTownOverlay->getSaturnPtr(0x0607B590);
            self->m10_state = 5;
            [[fallthrough]];
        case 5:
            if (primaryAnimationFinished(self)) self->m10_state = 0;
            break;
        case 6:
            self->getTask()->markFinished();
            break;
        }
    }

    static void Draw(sLagiCampInteractiveActor* self)
    {
        lagi::azel_bridge::begin_view_relative_submission_scope();
        pushCurrentMatrix();
        translateCurrentMatrix(self->m1C_position);
        rotateCurrentMatrixZYX(self->m28_rotation);
        self->m34_model.m18_drawFunction(&self->m34_model);
        if (!self->m18_paletteSequence.isNull())
        {
            translateCurrentMatrix(sVec3_FP{0, self->mE8_position[1] - self->m1C_position[1], 0});
            rotateCurrentMatrixShiftedY(self->mF4_rotation[1]);
            self->m100_model.m18_drawFunction(&self->m100_model);
        }
        popMatrix();
        lagi::azel_bridge::end_view_relative_submission_scope();
    }

    static void Delete(sLagiCampInteractiveActor* self)
    {
        if (self->m15_soundActive) playPCM(self, 0x64);
        auto& slot = npcData0.m70_npcPointerArray[readSaturnS32(self->mC_argument + 0x20)];
        if (slot.workArea == self) slot.workArea = nullptr;
    }

    sSaturnPtr mC_argument;
    s32 m10_state;
    s8 m14_sound, m15_soundActive, m16_variant;
    sSaturnPtr m18_paletteSequence;
    sVec3_FP m1C_position, m28_rotation;
    s_3dModel m34_model;
    sCollisionBody m84_body;
    sVec3_FP mE8_position, mF4_rotation;
    s_3dModel m100_model;
    sCollisionBody m150_body;
};

static sTownObject* lagiCreateCampInteractiveActor(npcFileDeleter* parent, sSaturnPtr arg)
{
    return createSubTaskWithArgWithCopy<sLagiCampInteractiveActor, sSaturnPtr>(parent, arg);
}

static s32 lagiCampInteractiveActorReady(s32 index)
{
    auto* actor = static_cast<sLagiCampInteractiveActor*>(getNpcDataByIndexAsTask(index));
    return actor->m10_state == 3;
}

static s32 lagiCampToggleInteractiveActor(s32 index)
{
    auto* actor = static_cast<sLagiCampInteractiveActor*>(getNpcDataByIndexAsTask(index));
    const s32 previous = actor->m10_state;
    actor->m10_state = previous == 3 ? 4 : 1;
    return previous;
}
