#pragma once
#include "kernel/vdp1AnimatedQuad.h"
// Host layout shared by the two generated D5 consumers. Native field names
// retain Saturn offsets; pointer-containing fields naturally widen on hosts.
struct s_nameEntryParticle {
    sVec3_FP m0_position;
    sAnimatedQuad mC_quad;
    fixedPoint m14_angle, m18_angleStep;
};
struct s_nameEntryParticleTask : s_workAreaTemplate<s_nameEntryParticleTask> {
    static const TypedTaskDefinition* getTypedTaskDefinition(){
        static const TypedTaskDefinition d={&Init,&Update,&Draw,nullptr};return &d;
    }
    static void Init(s_nameEntryParticleTask*);
    static void Update(s_nameEntryParticleTask*);
    static void Draw(s_nameEntryParticleTask*);
    s_fileBundle* m0_bundle{};
    u32 m4_characterArea{};
    fixedPoint m8{},mC_dx{},m10_dy{},m14_dz{},m18_amplitudeX{},m1C_amplitudeZ{},m20_scale{},m24_rotation{};
    u32 m28_state{},m2C_timer{};
    s_nameEntryParticle m30_particles[64];
};
s_nameEntryParticleTask* createParticleSystemTask(p_workArea parent);
