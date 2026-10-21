#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using u32=std::uint32_t;using u16=std::uint16_t;using s32=std::int32_t;
struct fixedPoint {
    s32 v=0;fixedPoint(s32 n=0):v(n){};operator s32()const{return v;}
    s32 asS32()const{return v;} fixedPoint& operator+=(s32 n){v+=n;return *this;}
};
struct sVec3_FP:std::array<fixedPoint,3>{};
struct sMatrix4x3{};struct s_fileBundle{};using p_workArea=void*;
template<class T>struct s_workAreaTemplate{struct TypedTaskDefinition{void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*del)(T*);};};
template<class T>T* createSubTask(p_workArea){return new T;}
struct sVdp1Quad{};struct sAnimatedQuad{const std::vector<sVdp1Quad>* m0_quad{};u16 m4_vdp1Memory{};unsigned m7_currentFrame{};};
static unsigned randomCalls=0,updates=0,draws=0;
u32 randomNumber(){return ++randomCalls*0x1234567u;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((std::int64_t(a.v)*b.v)>>16);}
fixedPoint getSin(unsigned){return 0;}fixedPoint getCos(unsigned){return 0x10000;}
void initMatrixToIdentity(sMatrix4x3*){}void rotateMatrixShiftedZ(fixedPoint,sMatrix4x3*){}
void transformAndAddVec(const sVec3_FP& a,sVec3_FP& b,const sMatrix4x3&){b=a;}
struct s_memoryAreaOutput{s_fileBundle* m0_mainMemoryBundle{};u32 m4_characterArea{};};
void getMemoryArea(s_memoryAreaOutput* p,unsigned i){assert(i==2);p->m4_characterArea=0x1000;}
struct Overlay{unsigned getSaturnPtr(unsigned a){assert(a==0x06078364);return a;}} overlay;
auto* gFLD_D5=&overlay;
std::vector<sVdp1Quad> initVdp1Quad(unsigned){return std::vector<sVdp1Quad>(16);}
void particleInitSub(sAnimatedQuad* p,u16 m,const std::vector<sVdp1Quad>* q){p->m0_quad=q;p->m4_vdp1Memory=m;}
int sGunShotTask_UpdateSub4(sAnimatedQuad*){++updates;return 1;}
struct Field{int m2C_currentFieldIndex=0;} field;
Field* getFieldTaskPtr(){return &field;}
struct Input{unsigned m8_newButtonDown=0;};
struct Device{Input m0_current;};struct Devices{Device m0_inputDevices[1];};
struct Graphics{Devices m4514;} graphicEngineStatus;
int vdp1DrawQuadScaled(sAnimatedQuad* q,sVec3_FP* p,fixedPoint scale){
    assert(q->m0_quad && q->m4_vdp1Memory==0x200 && scale.v>0);
    // The actual native global root reverses Z exactly once before clipping.
    assert(p->at(2).asS32()<0);++draws;return 1;
}
fixedPoint interpolateDistance(fixedPoint value,fixedPoint target,fixedPoint rate,fixedPoint maximum,s32 minimum){
    assert(target.v==0 && rate.v==0x1000);
    assert((maximum.v==0x1555 && minimum==32)||(maximum.v==0x800 && minimum==16));
    return value.v/2; // fixture response; production retains Azel's service
}
#include "d5_particle_fixture.inc"
int main(){
    s_nameEntryParticleTask t{};t.Init(&t);
    assert(randomCalls==256 && t.m10_dy.asS32()==-819);
    for(unsigned j=0;j<64;++j){
        const auto& p=t.m30_particles[j];
        assert(p.m0_position[0].asS32()==(s32(((j*4+1)*0x1234567u)&0x3ffff)-0x1ffff)*2);
        assert(p.m0_position[1].asS32()==int(64-j)*0x2000-0x40000);
        assert(p.m0_position[2].asS32()==-s32(((j*4+2)*0x1234567u)&0x1ffff)-0x20000);
        assert(p.mC_quad.m7_currentFrame==((((j*4+4)*0x1234567u)>>16)&15));
    }
    t.Update(&t);assert(updates==64 && t.m30_particles[0].m0_position[1].asS32()==0x40000-819);
    t.Draw(&t);assert(draws==64);
    t.m30_particles[0].m0_position[1]=-0x40001;t.Update(&t);
    assert(randomCalls==258 && t.m30_particles[0].m0_position[1].asS32()==0x40000);
    t.m28_state=2;t.m30_particles[0].m0_position[0]=10000;t.Update(&t);
    assert(t.m30_particles[0].m0_position[0].asS32()==5000);
    draws=0;t.Draw(&t);assert(draws==64); // not reduced-count fade approximation
    t.m28_state=3;t.m30_particles[0].m0_position[0]=10000;t.Update(&t);
    assert(t.m30_particles[0].m0_position[0].asS32()==15000);
    std::puts("PASS production D5 adapter RNG order, negative-Z layout, wrapping, 64 emissions and native state service calls");
}
