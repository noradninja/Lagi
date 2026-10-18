#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using u32=uint32_t; using s32=int32_t;
struct fixedPoint {
    s32 raw;
    fixedPoint(s32 n=0):raw(n){}
    fixedPoint operator+(fixedPoint b)const{return fixedPoint(u32(raw)+u32(b.raw));}
};
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return fixedPoint((int64_t(a.raw)*b.raw)>>16);}
struct sVec3_FP {std::array<fixedPoint,3> v;fixedPoint& operator[](unsigned i){return v[i];}};
struct s_workAreaCopy {virtual ~s_workAreaCopy()=default;};
using p_workArea=s_workAreaCopy*;
template<class T>struct s_workAreaTemplate:s_workAreaCopy{};
// Deliberately non-Saturn layout proves no raw parent+8 dependency survives.
struct sUrchin:s_workAreaCopy{std::array<char,97> padding; sVec3_FP m8;};
template<class T>T* createSubTaskFromFunction(s_workAreaCopy*,void(*)(T*)){return new T;}
std::vector<u32> rng;unsigned cursor;
u32 randomNumber(){assert(cursor<rng.size());return rng[cursor++];}
fixedPoint getSin(u32 a){assert(a<4096);return a==1024?65536:0;}
fixedPoint getCos(u32 a){assert(a<4096);return a==0?65536:0;}
s32 performModulo(s32 d,s32 v){return v%d;}
struct Data{u32 getSaturnPtr(u32 a){return a;}}data;Data* g_BTL_GenericData=&data;
struct sVdp1Quad{u32 address;};
unsigned reads;
std::vector<sVdp1Quad> initVdp1Quad(u32 a){++reads;return {{a}};}
struct File{} file;struct Entry{File* mC_fileBundle;};Entry dramAllocatorEnd[16];
unsigned emissions;const std::vector<sVdp1Quad>* retained; sVec3_FP captured;
void createParticleEffect(File* parent,const std::vector<sVdp1Quad>* quad,const sVec3_FP* pos,
    sVec3_FP* velocity,sVec3_FP* accel,s32 scale,const void* color,int lifetime){
    assert(parent==&file && !velocity && !accel && scale==65536 && !color && !lifetime);
    ++emissions;retained=quad;captured=*pos;
}
#include "lagi_battle_urchin_ambient.inc"
int main(){
    dramAllocatorEnd[12].mC_fileBundle=&file;
    sUrchin parent;parent.m8[0]=100;parent.m8[1]=200;parent.m8[2]=300;
    auto* task=static_cast<sNativeUrchinAmbient*>(lagiCreateUrchinAmbientEffect(&parent));
    assert(task->position==&parent.m8);
    rng={1};cursor=0;lagiUrchinAmbientUpdate(task);
    assert(cursor==1 && !emissions && !reads);
    const u32 tables[]={0x060A8300,0x060A84C0,0x060A87B4};
    for(unsigned selected=0;selected<3;++selected){
        rng={0,0,0,0,0,selected};cursor=0;lagiUrchinAmbientUpdate(task);
        assert(cursor==6 && retained->at(0).address==tables[selected]);
        assert(captured[0].raw==100 && captured[1].raw==200 && captured[2].raw==300);
    }
    assert(reads==3);
    parent.m8[1]=900;
    rng={0,0x08000000,0x04000000,0x08000000,0,2};cursor=0;lagiUrchinAmbientUpdate(task);
    assert(cursor==6 && captured[0].raw==100 && captured[1].raw==900+0xA000 && captured[2].raw==300+0xA000);
    rng={0,0,0x04000000,0,0x04000000,1};cursor=0;lagiUrchinAmbientUpdate(task);
    assert(cursor==6 && captured[0].raw==100+0xA000 && captured[1].raw==900+0xA000 && captured[2].raw==300);
    delete task;
    assert(retained->at(0).address==tables[1]);
    std::puts("PASS production native emitter gate/RNG/position/trig/quad/resource ownership and descriptor lifetime; not Vita execution");
}
