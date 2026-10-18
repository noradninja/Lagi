#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s32=int32_t;using s16=int16_t;using u16=uint16_t;using u8=uint8_t;
struct sSaturnPtr{unsigned address; sSaturnPtr operator+(unsigned n)const{return {address+n};}};
struct sVec3_FP{std::array<int,3> v{};};
struct sVdp1Quad{};struct sAnimatedQuad{const std::vector<sVdp1Quad>* frames{};};
struct sAnimationData{int m4_numFrames=3;}animation;
struct Pose{}pose;
struct Hierarchy{unsigned countNumberOfBones(){return 7;}}hierarchy;
struct s_fileBundle{
    Hierarchy* getModelHierarchy(u16 n){assert(n==4);return &hierarchy;}
    sAnimationData* getAnimation(u16 n){assert(n==0x30);return &animation;}
    Pose* getStaticPose(u16 n,unsigned bones){assert(n==0x34 && bones==7);return &pose;}
}bundle;
struct Allocation{u16 m4_vdp1Memory=0x1234;}allocation;
struct Control{bool finished=false;void markFinished(){finished=true;}};
struct s_workArea{virtual ~s_workArea()=default;};using p_workArea=s_workArea*;
struct Owner:s_workArea{s_fileBundle* m0_fileBundle=&bundle;Allocation* m4_vd1Allocation=&allocation;}owner;
struct Entry{Owner* mC_fileBundle;};Entry dramAllocatorEnd[16];
template<class T>struct s_workAreaTemplateWithCopy:Owner{
    struct TypedTaskDefinition{void* init;void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};
    void(*m_UpdateMethod)(T*){};void(*m_DrawMethod)(T*){};Control control;
    Control* getTask(){return &control;}
};
template<class T>T* createSubTaskWithCopy(Owner* p,const typename T::TypedTaskDefinition* d){
    assert(p==&owner);auto* t=new T;t->m_UpdateMethod=d->update;t->m_DrawMethod=d->draw;return t;
}
unsigned destroyed=0,modelDraws=0,modelSteps=0,quadSteps=0,quadInits=0,quadDraws=0,clockReads=0;
struct s_3dModel{
    sAnimationData* m30_pCurrentAnimation{};int frame{};
    void(*m18_drawFunction)(s_3dModel*){};
    ~s_3dModel(){++destroyed;}
};
int remaining=100;bool stopped=false;std::vector<int> clockSamples;
s32 e006_scriptFunction_6057438(){++clockReads;if(clockSamples.empty())return remaining;const auto value=clockSamples.front();clockSamples.erase(clockSamples.begin());return value;}
s32 e006_scriptFunction_605861eSub0(){return stopped;}
s16 readSaturnS16(sSaturnPtr p){
    if(p.address==0x1000 || p.address==0x2000)return 2;
    if(p.address==0x1014 || p.address==0x2022)return 2;
    assert(false);return 0;
}
u16 readSaturnU16(sSaturnPtr p){
    switch(p.address){case 0x201C:return 4;case 0x201E:return 0x30;case 0x2020:return 0x34;}
    assert(false);return 0;
}
sSaturnPtr readSaturnEA(sSaturnPtr p){assert(p.address==0x1010);return {0x3000};}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {{int(p.address),2,3}};}
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr p){assert(p.address==0x3000);return std::vector<sVdp1Quad>(2);}
void particleInitSub(sAnimatedQuad* q,u16 vram,const std::vector<sVdp1Quad>* frames){
    assert(vram==0x1234 && frames->size()==2);q->frames=frames;++quadInits;
}
int sGunShotTask_UpdateSub4(sAnimatedQuad* q){assert(q->frames);++quadSteps;return 2;}
void drawProjectedParticle(sAnimatedQuad* q,sVec3_FP* p){assert(q->frames && p->v[0]==0x1004);++quadDraws;}
bool initOK=true;
bool init3DModelRawData(s_workArea*,s_3dModel* m,unsigned flags,s_fileBundle* f,u16 index,
    sAnimationData* a,Pose* p,void* color,void* hotpoints){
    assert(!flags && f==&bundle && index==4 && a==&animation && p==&pose && !color && !hotpoints);
    m->m30_pCurrentAnimation=a;m->m18_drawFunction=[](s_3dModel*){++modelDraws;};return initOK;
}
unsigned stepAnimation(s_3dModel* m){++modelSteps;const auto frame=m->frame;m->frame=(m->frame+1)%3;return frame;}
int matrixDepth=0;void pushCurrentMatrix(){++matrixDepth;}void popMatrix(){--matrixDepth;}
void translateCurrentMatrix(const sVec3_FP& p){assert(matrixDepth==1 && p.v[0]==0x2004);}
void rotateCurrentMatrixZYX(const sVec3_FP& r){assert(matrixDepth==1 && r.v[0]==0x2010);}
#include "lagi_e011_scheduled_effects.inc"
int main(){
    s16 previous=101;clockSamples={100,99};
    assert(lagiE011EffectTicks(previous)==1 && previous==99 && clockSamples.empty());
    dramAllocatorEnd[2].mC_fileBundle=&owner;
    auto* q=static_cast<sE011ScheduledQuad*>(lagiE011CreateScheduledQuad({0x1000}));
    assert(q->previous==101 && q->cycles==2);
    lagiE011QuadUpdate(q);
    assert(quadInits==1 && quadSteps==0 && q->state==1 && q->cycles==2);
    lagiE011QuadDraw(q);assert(quadDraws==1);
    lagiE011QuadUpdate(q);assert(quadSteps==0 && q->state==1 && q->cycles==2);
    --remaining;lagiE011QuadUpdate(q);assert(quadSteps==1 && q->cycles==1);
    --remaining;lagiE011QuadUpdate(q);assert(quadSteps==2 && q->state==2 && !q->control.finished);
    --remaining;lagiE011QuadUpdate(q);assert(q->control.finished);
    lagiE011QuadDelete(q);assert(!q->frames);delete q;
    remaining=100;
    auto* m=static_cast<sE011ScheduledModel*>(lagiE011CreateScheduledModel({0x2000}));
    const auto clockBefore=clockReads;
    lagiE011ModelUpdate(m);assert(modelSteps==1 && m->cycles==2 && clockReads==clockBefore+2);
    remaining-=2;lagiE011ModelUpdate(m);assert(modelSteps==3 && m->cycles==1);
    lagiE011ModelUpdate(m);assert(modelSteps==3 && m->cycles==1 && m->state==1);
    remaining-=3;lagiE011ModelUpdate(m);assert(modelSteps==6 && m->state==2);
    lagiE011ModelDraw(m);assert(matrixDepth==0 && modelDraws==1);
    const auto before=clockReads;stopped=true;lagiE011ModelUpdate(m);
    assert(m->control.finished && before==clockReads);
    lagiE011ModelDelete(m);delete m;assert(destroyed==1);
    stopped=false;initOK=false;
    m=static_cast<sE011ScheduledModel*>(lagiE011CreateScheduledModel({0x2000}));
    assert(m->control.finished && !m->m_DrawMethod && !m->m_UpdateMethod);
    lagiE011ModelDelete(m);delete m;assert(destroyed==2);
    std::puts("PASS production E011 scheduled quad/model creation, native clock tick counts, loops, drawing, cleanup and init failure; dispatcher not connected; not Vita playback");
}
