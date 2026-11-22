#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/native_field_cells.h"
#include "lagi/garil_mound.h"
#include "field/field_a5/o_fld_a5.h"
#include "field/field_a3/o_fld_a3.h"
#include "field/fieldModelRender.h"
#include "field/fieldVisibilityGrid.h"
#include "field/fieldItemBox.h"
#include "kernel/vdp1AnimatedQuad.h"
#include "kernel/fileBundle.h"
#include "3dModels.h"
#include <array>
#include <map>

// Existing Valley service: original A5 060703E4 is the same dispatch contract.
void startBaldorQueenBattle(int encounter, int completionBit);
void lagiApplyA5WindToParticle(sVec3_FP*);
int lagiDrawPoolParticle(sAnimatedQuad*, sVec3_FP*, fixedPoint, fixedPoint);

namespace {
const std::vector<sVdp1Quad>* moundQuads(unsigned address) {
    static std::map<unsigned, std::vector<sVdp1Quad>> cache;
    auto [it, inserted] = cache.try_emplace(address);
    if (inserted) it->second = initVdp1Quad(gFLD_A5->getSaturnPtr(address));
    return &it->second;
}

// Native 06055F6C/06056080/060560DA: 32 particles, seven replacement attempts.
struct MoundSand : s_workAreaTemplateWithArg<MoundSand, std::pair<sVec3_FP, fixedPoint>> {
    struct Particle {
        sAnimatedQuad quad;
        sVec3_FP position, velocity;
        fixedPoint scale;
        s16 life = 0;
    };
    s_memoryAreaOutput memory;
    std::array<Particle, 32> particles;
    sVec3_FP origin;
    fixedPoint initialScale;
    int replacements = 7;
    static void Reset(MoundSand* self, Particle& p) {
        p.position = self->origin;
        for (unsigned i=0;i<3;++i) p.position[i] += (randomNumber() & 0x3FFF) - 0x1FFF;
        p.velocity.m0_X = (randomNumber() & 0xFFF) - 0x7FF;
        p.velocity.m8_Z = (randomNumber() & 0xFFF) - 0x7FF;
        p.velocity.m4_Y = randomNumber() & 0xFFF;
        p.life = randomNumber() & 0x1F;
        p.scale = self->initialScale;
        // Native Reset changes the frame table only, not the animation counters.
        p.quad.m0_quad = moundQuads(0x0608D480);
    }
    static void Init(MoundSand* self, std::pair<sVec3_FP, fixedPoint> arg) {
        getMemoryArea(&self->memory, 2);
        self->origin = arg.first; self->initialScale = arg.second;
        const u16 vram = (self->memory.m4_characterArea - 0x25C00000) >> 3;
        for (auto& p : self->particles) {
            Reset(self, p);
            particleInitSub(&p.quad, vram, moundQuads(0x0608D480));
            const unsigned advance = randomNumber() & 0xF;
            for (unsigned i=0;i<advance;++i) sGunShotTask_UpdateSub4(&p.quad);
        }
    }
    static void Update(MoundSand* self) {
        if (self->replacements) {
            --self->replacements;
            for (auto& p : self->particles) if (!p.life) { Reset(self,p); break; }
        }
        bool active = false;
        for (auto it=self->particles.rbegin();it!=self->particles.rend();++it) {
            auto& p = *it;
            if (!p.life) continue;
            active = true;
            const auto oldLife = p.life--;
            if (oldLife < 5) p.quad.m0_quad = moundQuads(0x0608D5D0);
            sGunShotTask_UpdateSub4(&p.quad);
            p.velocity.m4_Y -= 0x2C;
            p.position += p.velocity;
            p.scale = MTH_Mul(p.scale, 0x111EB);
            lagiApplyA5WindToParticle(&p.position);
        }
        if (!active) self->getTask()->markFinished();
    }
    static void Draw(MoundSand* self) {
        for (auto& p : self->particles) if (p.life)
            lagiDrawPoolParticle(&p.quad, &p.position, p.scale, fixedPoint(-0x6000));
    }
};
void spawnMoundSand(s_workArea* parent, const sVec3_FP& position, fixedPoint scale) {
    static const MoundSand::TypedTaskDefinition definition = {
        MoundSand::Init, MoundSand::Update, MoundSand::Draw, nullptr};
    createSubTaskWithArg<MoundSand>(parent, std::make_pair(position, scale), &definition);
}

// Original 0608D72C task definition, work area 0x64. Host offsets are named.
struct Mound : s_workAreaTemplateWithArg<Mound, s_DataTable2Sub0*> {
    s_memoryAreaOutput memory;
    s_DataTable2Sub0* placement = nullptr;
    sFieldModelRenderContext target;
    sVec3_FP targetPosition, sandPosition;
    int timer = 0;
    s16 collisionModel = 0, collisionPose = 0;
    unsigned kind = 0;
    u8 moving = 0, frame = 0, cycles = 0;
    static void Target(Mound*) {
        // Native 06055D50: only a failed script start dispatches immediately.
        if (!startFieldScript(0, 0x5B5)) startBaldorQueenBattle(0x10, 0x90);
    }
    static void Init(Mound* self, s_DataTable2Sub0* placement) {
        self->placement = placement;
        self->timer = performModulo2(300, randomNumber());
        self->targetPosition = placement->m4_position;
        self->targetPosition.m4_Y += 0x10000;
        initFieldModelRenderContext(&self->target, self, reinterpret_cast<void*>(&Target),
            &self->targetPosition, nullptr, 1, 0, -1, 0, 0);
        self->target.m18_visibilityFlags |= 1;
        self->sandPosition = placement->m4_position;
        switch (readSaturnS16(placement->m1C_modelData)) {
        case 0x1E8:
            self->kind=0; self->collisionModel=0x1F4; self->collisionPose=0x728;
            self->sandPosition.m4_Y+=0xE000; break;
        case 0x1EC:
            self->kind=1; self->collisionModel=0x1F8; self->collisionPose=0x72C;
            self->sandPosition.m4_Y+=0xA000; break;
        case 0x1F0:
            self->kind=2; self->collisionModel=0x1FC; self->collisionPose=0x730;
            self->sandPosition.m4_Y+=0x16000;
            self->target.m18_visibilityFlags=0; break;
        default: assert(false); break;
        }
    }
    static void Update(Mound* self) {
        updateFieldModelRenderContext(&self->target);
        // Native 06055BF6: authored victory bit also retires decorative mounds.
        if (mainGameState.bitField[0x12] & 0x80) self->getTask()->markFinished();
        if (self->timer && !--self->timer) {
            sVec3_FP view;
            transformAndAddVecByCurrentMatrix(&self->sandPosition, &view);
            if (isPointOnScreen(&view,0x100000)) {
                self->frame=0; self->cycles=3; ++self->moving;
            } else self->timer=240+performModulo2(600,randomNumber());
        }
        if (self->moving) {
            const auto oldFrame=self->frame++;
            if (oldFrame==28) spawnMoundSand(self,self->sandPosition,
                readSaturnFP(gFLD_A5->getSaturnPtr(0x0608D720+self->kind*4)));
            if (self->frame>=39) {
                self->frame=0;
                if (!--self->cycles) {
                    self->moving=0;
                    self->timer=240+performModulo2(600,randomNumber());
                }
            }
        }
    }
    static void Draw(Mound* self) {
        auto& placement=*self->placement;
        if (checkPositionVisibilityAgainstFarPlane(&placement.m4_position)) return;
        pushCurrentMatrix(); translateCurrentMatrix(&placement.m4_position);
        rotateCurrentMatrixZYX_s16(placement.m10_rotation);
        auto model=placement.m1C_modelData;
        if (self->moving) model=readSaturnEA(gFLD_A5->getSaturnPtr(0x0608D474+self->kind*4))+self->frame*4;
        LCSItemBox_DrawType0Sub0(self->memory.m0_mainMemoryBundle,
            readSaturnS16(model),readSaturnS16(model+2));
        // Original 0607767E is the shared transformed collision producer.
        LCSItemBox_UpdateType0Sub0(self->memory.m0_mainMemoryBundle,
            self->collisionModel,self->collisionPose,0x10000);
        popMatrix();
    }
};
void createMound(s_visdibilityCellTask* cell, s_DataTable2Sub0& placement, int memoryArea) {
    static const Mound::TypedTaskDefinition definition={Mound::Init,Mound::Update,Mound::Draw,nullptr};
    if (auto* actor=createSubTaskWithArg<Mound>(cell,&placement,&definition))
        getMemoryArea(&actor->memory,memoryArea);
}
}
void lagiSetupGarilMoundField(s_DataTable3* environment, void(*startTasks)(s_workArea*)) {
    lagiRegisterNativeFieldCellFactory(gFLD_A5,0x06055E1E,createMound);
    // Native 060543B6..060543C0 -> 06078D26: preserve BOTH original tables.
    setupFieldWithCellObjects(environment,readDataTable2(gFLD_A5->getSaturnPtr(0x0608E34C)),startTasks);
}
