#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int; using u16=std::uint16_t; using sVec3_FP=std::array<int,3>;
struct Animation { int m4_numFrames=4; } animation;
struct Hierarchy { int countNumberOfBones(){return 3;} } hierarchy;
struct Bundle {
 Hierarchy* getModelHierarchy(u16 i){assert(i==0x18);return &hierarchy;}
 Animation* getAnimation(u16 i){assert(i==0x210);return &animation;}
 void* getStaticPose(u16 i,int bones){assert(i==0x1CC&&bones==3);return this;}
} bundle;
struct npcFileDeleter{} owner;
struct Task { bool finished=false; void markFinished(){finished=true;} } state;
template<class T> struct s_workAreaTemplateWithCopy {
 struct TypedTaskDefinition {void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
 Bundle* m0_fileBundle=&bundle; void(*m_UpdateMethod)(T*){};void(*m_DrawMethod)(T*){};
 Task* getTask(){return &state;}
};
int draws=0,pushes=0,pops=0,liveModels=0; bool initSuccess=true;
void drawModel(struct s_3dModel*);
struct s_3dModel {
 Animation* m30_pCurrentAnimation=&animation; int m10_currentAnimationFrame=0;
 void(*m18_drawFunction)(s_3dModel*)=drawModel;
 s_3dModel(){++liveModels;} ~s_3dModel(){--liveModels;}
};
void drawModel(s_3dModel*){++draws;}
void stepAnimation(s_3dModel* m){++m->m10_currentAnimationFrame;}
void pushCurrentMatrix(){++pushes;}void popMatrix(){++pops;}
void translateCurrentMatrix(const sVec3_FP& p){assert(p==sVec3_FP({1,2,3}));}
void rotateCurrentMatrixYXZ(const sVec3_FP& p){assert(p==sVec3_FP({4,5,6}));}
void* allocated=nullptr;
template<class T> T* createSubTaskWithCopy(npcFileDeleter* p,const typename T::TypedTaskDefinition* d){
 assert(p==&owner); auto* t=new T; t->m_UpdateMethod=d->update;t->m_DrawMethod=d->draw;allocated=t;return t;
}
template<class T> bool init3DModelRawData(T*,s_3dModel*,int mode,Bundle* b,u16 index,Animation* a,void* pose,void*,void*){
 assert(mode==0&&b==&bundle&&index==0x18&&a==&animation&&pose==&bundle);return initSuccess;
}
#include "../src/integration/lagi_arachnoth_impact.inc"
int main(){
 sVec3_FP p{1,2,3},r{4,5,6};
 // SH-2 76BC sign-extends the immediate: 0x210 + (-68), not +188.
 constexpr u16 nativePose = 0x210 + static_cast<std::int8_t>(0xBC);
 static_assert(nativePose == 0x1CC);
 lagiCreateBattleImpactModel(&owner,0x18,nativePose,0x210,p,r);
 auto* t=static_cast<sLagiBattleImpactModel*>(allocated);p[0]=99;r[0]=99;
 lagiBattleImpactDraw(t);assert(draws==1&&pushes==pops);
 lagiBattleImpactUpdate(t);lagiBattleImpactUpdate(t);assert(!state.finished);
 lagiBattleImpactUpdate(t);assert(state.finished);
 lagiBattleImpactDelete(t);delete t;assert(liveModels==0);
 state.finished=false;initSuccess=false;
 lagiCreateBattleImpactModel(&owner,0x18,nativePose,0x210,p,r);
 t=static_cast<sLagiBattleImpactModel*>(allocated);
 assert(state.finished&&!t->m_UpdateMethod&&!t->m_DrawMethod);
 lagiBattleImpactDelete(t);delete t;assert(liveModels==0);
 puts("PASS production impact model: native resource IDs, copied transform, animation completion, draw and cleanup; not device acceptance");
}
