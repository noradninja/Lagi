#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <vector>
using s8=std::int8_t; using s16=std::int16_t; using s32=std::int32_t; using u16=std::uint16_t;
struct sSaturnPtr {
 s32 address=0;
 sSaturnPtr operator+(s32 n)const{return {address+n};}
 sSaturnPtr& operator+=(s32 n){address+=n;return *this;}
 bool isNull()const{return !address;}
 static sSaturnPtr getNull(){return {};}
};
using sVec3_FP=std::array<s32,3>;
std::map<s32,s32> memory;
s32 readSaturnS32(sSaturnPtr p){return memory[p.address];}
u16 readSaturnU16(sSaturnPtr p){return memory[p.address];}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {memory[p.address]};}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {memory[p.address],memory[p.address+4],memory[p.address+8]};}
struct sAnimationData{u16 m4_numFrames=3;};
struct s_3dModel{ sAnimationData* m30_pCurrentAnimation=nullptr; void(*m18_drawFunction)(s_3dModel*)=nullptr; int model=0;};
struct Bundle {
 struct Hierarchy{int countNumberOfBones(){return 4;}} hierarchy;
 struct Pose{} pose;
 sAnimationData animation;
 Hierarchy* getModelHierarchy(int){return &hierarchy;}
 Pose* getStaticPose(int,int){return &pose;}
 sAnimationData* getAnimation(int){return &animation;}
};
struct Task{bool finished=false;void markFinished(){finished=true;}};
struct s_workArea{virtual ~s_workArea()=default;};
struct sTownObject:s_workArea{Bundle* m0_fileBundle=nullptr;};
struct npcFileDeleter:sTownObject{};
template<class T,class Base,class Arg>struct s_workAreaTemplateWithArgAndBase:Base {
 struct TypedTaskDefinition{void(*init)(T*,Arg);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
 void(*m_UpdateMethod)(T*)=nullptr;void(*m_DrawMethod)(T*)=nullptr;
 Task task;Task* getTask(){return &task;}
};
template<class T,class Arg>T* createSubTaskWithArgWithCopy(npcFileDeleter* p,Arg a){auto*t=new T;t->m0_fileBundle=p->m0_fileBundle;T::Init(t,a);return t;}
struct sCollisionBody{sVec3_FP* m30_pPosition=nullptr; sVec3_FP* m34_pRotation=nullptr;s_workArea* m38_pOwner=nullptr;sSaturnPtr m3C_scriptEA;void* m40=nullptr;int setup=0;};
struct Slot{s_workArea* workArea=nullptr;void* pNPC=nullptr;};
struct{std::array<Slot,16>m70_npcPointerArray;}npcData0;
s_workArea* getNpcDataByIndexAsTask(s32 n){return npcData0.m70_npcPointerArray[n].workArea;}
struct Overlay{sSaturnPtr getSaturnPtr(s32 a){return {a};}}overlay;
Overlay* gCurrentTownOverlay=&overlay;
bool loaded=false;int modelInits=0,draws=0,primarySteps=0,secondarySteps=0; s16 frame=0;
s_3dModel* primary=nullptr;std::vector<int>sounds;std::vector<sCollisionBody*>collisions;std::vector<int>palettes;
void draw(s_3dModel*){++draws;}
void init3DModelRawData(s_workArea*,s_3dModel*m,int,Bundle*,u16 model,sAnimationData*a,Bundle::Pose*,void*,void*){++modelInits;m->model=model;m->m30_pCurrentAnimation=a;m->m18_drawFunction=&draw;}
bool isDataLoaded(int){return loaded;}
s16 updateAndInterpolateAnimation(s_3dModel*m){if(m==primary)++primarySteps;else ++secondarySteps;return frame;}
void initAnimation(s_3dModel*m,sAnimationData*a){m->m30_pCurrentAnimation=a;}
void setCollisionSetup(sCollisionBody*b,int n){b->setup=n;}
void setCollisionBounds(sCollisionBody*,sVec3_FP,sVec3_FP){}
void registerCollisionBody(sCollisionBody*b){collisions.push_back(b);}
void playPCM(s_workArea*,int n){sounds.push_back(n);}
void* getVdp2Cram(int n){assert(n==0x240);return nullptr;}
void asyncDmaCopy(sSaturnPtr p,void*,int size,int mode){assert(size==0x20&&mode==0);palettes.push_back(p.address);}
int scopes=0,stack=0;
namespace lagi::azel_bridge{void begin_view_relative_submission_scope(){++scopes;}void end_view_relative_submission_scope(){--scopes;}}
void pushCurrentMatrix(){++stack;}void popMatrix(){--stack;}void translateCurrentMatrix(sVec3_FP){}void rotateCurrentMatrixZYX(sVec3_FP){}void rotateCurrentMatrixShiftedY(int){}
#include "production.inc"
int main(){
 using Actor=sLagiCampInteractiveActor;
 const sSaturnPtr arg{0x607BA7C};memory[arg.address]=7;memory[arg.address+0x20]=8;
 memory[arg.address+8]=11;memory[arg.address+12]=22;memory[arg.address+16]=33;
 memory[arg.address+0x24]=0x605AAAA;memory[arg.address+0x28]=0x605BBBB;
 for(int i=0;i<7;++i){memory[0x607B308+i*6]=0x28+i*4;memory[0x607B30A+i*6]=0x45C+i*4;memory[0x607B30C+i*6]=0x4D0;}
 Bundle bundle;npcFileDeleter parent;parent.m0_fileBundle=&bundle;
 auto* a=static_cast<Actor*>(lagiCreateCampInteractiveActor(&parent,arg));primary=&a->m34_model;
 assert(npcData0.m70_npcPointerArray[8].workArea==a);
 assert(a->mE8_position[1]==22+0x1333&&a->m10_state==0);
 Actor::AwaitResource(a);assert(modelInits==0&&a->m_DrawMethod==nullptr);
 loaded=true;Actor::AwaitResource(a);assert(modelInits==2&&a->m_UpdateMethod==&Actor::Update&&a->m_DrawMethod==&Actor::Draw);
 assert(a->m84_body.m30_pPosition==&a->m1C_position&&a->m150_body.m30_pPosition==&a->mE8_position);
 assert(a->m84_body.m3C_scriptEA.address==0x605AAAA&&a->m150_body.m3C_scriptEA.address==0x605BBBB);
 Actor::Draw(a);assert(draws==1&&!stack&&!scopes);
 for(int cycle=0;cycle<8;++cycle){
   assert(lagiCampToggleInteractiveActor(8)==0);assert(a->m10_state==1);
   frame=0;Actor::Update(a);assert(a->m10_state==2&&a->m100_model.model==0x28+(cycle%7)*4);
   frame=2;Actor::Update(a);assert(lagiCampInteractiveActorReady(8)==1&&a->m15_soundActive==1);
   assert(sounds.back()==101+(cycle%2));
   // One palette, followed by a zero marker and negative relative loop.
   memory[0x607B534]=0x607B332;memory[0x607B538]=0;memory[0x607B53C]=-1;
   const auto yaw=a->mF4_rotation[1];Actor::Update(a);Actor::Update(a);
   assert(palettes.back()==0x607B332&&a->mF4_rotation[1]==yaw-2*0x5B05B);
   Actor::Draw(a);assert(!stack&&!scopes);
   assert(lagiCampToggleInteractiveActor(8)==3&&a->m10_state==4);
   frame=0;Actor::Update(a);assert(a->m10_state==5&&!a->m15_soundActive&&sounds.back()==100);
   memory[0x607B590]=0;memory[0x607B594]=0;frame=2;Actor::Update(a);
   assert(a->m10_state==0&&a->m18_paletteSequence.isNull());
 }
 // Signed frame comparison with no current animation is still native completion.
 a->m10_state=2;a->m34_model.m30_pCurrentAnimation=nullptr;frame=-1;Actor::Update(a);assert(a->m10_state==3);
 a->m18_paletteSequence={};a->m10_state=6;Actor::Update(a);assert(a->getTask()->finished);
 const auto n=sounds.size();Actor::Delete(a);assert(sounds.size()==n+1&&sounds.back()==100&&npcData0.m70_npcPointerArray[8].workArea==nullptr);
 npcData0.m70_npcPointerArray[8].workArea=&parent;Actor::Delete(a);assert(npcData0.m70_npcPointerArray[8].workArea==&parent);
 delete a;std::puts("Camp interactive actor production lifecycle passed");
}
