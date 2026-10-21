#include "PDS.h"
#include "lagi/lagi_d5_particles.h"
#include "field/field_d5/o_fld_d5.h"
#include "field.h"
#include "trigo.h"

fixedPoint interpolateDistance(fixedPoint,fixedPoint,fixedPoint,fixedPoint,s32);
static std::vector<sVdp1Quad> d5ParticleFrames;
// FLD_D5 060547F8/06054872. Negative Z is authored object space; the
// native projection root flips Z. Positive random Z clipped every particle.
static void resetD5Particle(s_nameEntryParticle& p, int index, bool initial){
    p.m0_position[0]=((s32)(randomNumber()&0x3ffff)-0x1ffff)*2;
    p.m0_position[1]=initial ? index*0x2000-0x40000 : 0x40000;
    p.m0_position[2]=-(s32)(randomNumber()&0x1ffff)-0x20000;
    if(initial){
        p.m14_angle=(s32)(randomNumber()&0x3ff);
        p.m18_angleStep=MTH_Mul(fixedPoint(0x222222),fixedPoint(index*0x400));
    }
}
// 06054A68: retain native RNG call ordering and the resource-authored frames.
void s_nameEntryParticleTask::Init(s_nameEntryParticleTask* t){
    s_memoryAreaOutput area;getMemoryArea(&area,2);
    t->m0_bundle=area.m0_mainMemoryBundle;t->m4_characterArea=area.m4_characterArea;
    t->m8=0x111111;t->mC_dx=0;t->m10_dy=-819;t->m14_dz=0;
    t->m18_amplitudeX=t->m1C_amplitudeZ=0x4000;t->m20_scale=0x10000;
    if(d5ParticleFrames.empty())d5ParticleFrames=initVdp1Quad(gFLD_D5->getSaturnPtr(0x06078364));
    // The field service exposes an offset, rather than Saturn's 25C00000 base.
    const u16 memory=static_cast<u16>(area.m4_characterArea>>3);
    for(int i=64;i>0;--i){
        auto& p=t->m30_particles[64-i];resetD5Particle(p,i,true);
        particleInitSub(&p.mC_quad,memory,&d5ParticleFrames);
        p.mC_quad.m7_currentFrame=(randomNumber()>>16)&15;
    }
}
// 060548A4 / 06054AE4: all 64 particles survive the state transitions.
void s_nameEntryParticleTask::Update(s_nameEntryParticleTask* t){
    for(auto& p:t->m30_particles){
        p.m14_angle=(p.m14_angle.asS32()+p.m18_angleStep.asS32())&0x0fffffff;
        switch(t->m28_state){
        case 0:case 1:{
            const int n=t->m28_state==1?2:1;
            p.m0_position[0]+=t->mC_dx*n;
            p.m0_position[1]+=t->m10_dy*n;
            p.m0_position[2]+=t->m14_dz*n;break;
        }
        case 2:case 3:{
            for(int axis=0;axis<2;++axis){
                const auto value=p.m0_position[axis];
                const auto toward=interpolateDistance(value,0,0x1000,0x1555,0x20);
                if(t->m28_state==2)p.m0_position[axis]=toward;
                else p.m0_position[axis]=value+(value-toward);
            }
            p.m0_position[2]+=t->m14_dz;break;
        }
        }
        if(p.m0_position[1]<-0x40000)resetD5Particle(p,0,false);
        sGunShotTask_UpdateSub4(&p.mC_quad);
    }
}
// 060549DC / 06054B16: identity local matrix in name entry, native global
// projection exactly once. Other D5 subfields rotate the local matrix in Z.
void s_nameEntryParticleTask::Draw(s_nameEntryParticleTask* t){
    const auto field=getFieldTaskPtr()->m2C_currentFieldIndex;
    if(field==0){
        if(t->m28_state==0 &&
           (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown&6))
            t->m28_state=1;
        else if(t->m28_state==1){
            const unsigned phase=((t->m2C_timer*0x444444u)>>16)&0xfff;
            t->m20_scale=0x10000+MTH_Mul(getSin(phase),fixedPoint(0x8000));
            if(++t->m2C_timer>=30){t->m28_state=0;t->m2C_timer=0;}
        }
    }
    if(t->m28_state==2 || t->m28_state==3){
        t->m18_amplitudeX=interpolateDistance(t->m18_amplitudeX,0,0x1000,0x800,16);
        t->m1C_amplitudeZ=interpolateDistance(t->m1C_amplitudeZ,0,0x1000,0x800,16);
    }
    sMatrix4x3 local;initMatrixToIdentity(&local);
    if(field!=0){t->m24_rotation+=0x88888;rotateMatrixShiftedZ(t->m24_rotation,&local);}
    for(auto& p:t->m30_particles){
        sVec3_FP position=p.m0_position,transformed;
        const unsigned phase=(u32(p.m14_angle.asS32())>>16)&0xfff;
        position[0]+=MTH_Mul(getSin(phase),t->m18_amplitudeX);
        position[2]+=MTH_Mul(getCos(phase),t->m1C_amplitudeZ);
        transformAndAddVec(position,transformed,local);
        if(position[1]>=-0x40000)vdp1DrawQuadScaled(&p.mC_quad,&transformed,t->m20_scale);
    }
}
s_nameEntryParticleTask* createParticleSystemTask(p_workArea parent){return createSubTask<s_nameEntryParticleTask>(parent);}
