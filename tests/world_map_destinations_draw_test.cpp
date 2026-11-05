#include <array>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u8=std::uint8_t;
struct fixedPoint {s32 v=0;fixedPoint(s32 n=0):v(n){}s32 asS32()const{return v;}};
using sVec3_FP=std::array<fixedPoint,3>;
struct sSaturnPtr {int ea=0;bool isNull()const{return !ea;}sSaturnPtr operator+(int n)const{return {ea+n};}};
struct Record {s16 gate=-1;int type=0;sVec3_FP position{};};
std::array<Record,50> records;
s16 readSaturnS16(sSaturnPtr p){assert(p.ea>=0x10000);return records[(p.ea-0x10000)/36].gate;}
int readSaturnS8(sSaturnPtr p){assert((p.ea-0x10000)%36==5);return records[(p.ea-0x10000)/36].type;}
sVec3_FP readSaturnVec3(sSaturnPtr p){
 if(p.ea==0x4CAD)return {1,2,3};
 if(p.ea==0x4C95)return {4,5,6};
 assert((p.ea-0x10000)%36==8);return records[(p.ea-0x10000)/36].position;
}
struct {std::array<u8,32> bitField{};bool getBit(unsigned bit){return bitField[bit/8]&(0x80>>(bit%8));}}mainGameState;
struct sVdp1Quad {};
struct sAnimatedQuad {const std::vector<sVdp1Quad>* frames{};};
struct s_3dModel {s16 m14=0;void(*m18_drawFunction)(s_3dModel*){};};
bool hierarchyPresent=true,posePresent=true,initSucceeds=true;
struct Hierarchy {int countNumberOfBones(){return 2;}} hierarchy;
struct Pose {}pose;
struct Model {}model;
struct s_fileBundle {
 Hierarchy* getModelHierarchy(unsigned slot){assert(slot==0x1E8);return hierarchyPresent?&hierarchy:nullptr;}
 Pose* getStaticPose(unsigned slot,int n){assert(slot==0x5C4&&n==2);return posePresent?&pose:nullptr;}
 u8* getRawFileAtOffset(unsigned slot){assert(slot==0x5CC);return reinterpret_cast<u8*>(1);}
 Model* get3DModel(unsigned slot){assert(slot==0x3CC);return &model;}
}bundle;
struct Allocation {unsigned m4_vdp1Memory=0x1234;}allocation;
struct s_workAreaCopy {s_fileBundle* m0_fileBundle=&bundle;Allocation* m4_vd1Allocation=&allocation;}owner;
std::vector<int> calls;
int ticks=0,steps=0,depth=0;
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr p){assert(p.ea==0x4B45);return {{}};}
void particleInitSub(sAnimatedQuad* q,unsigned memory,const std::vector<sVdp1Quad>* frames){assert(memory==0x1234);q->frames=frames;}
void drawModel(s_3dModel* m){calls.push_back(100+m->m14);}
bool init3DModelRawData(s_workAreaCopy* o,s_3dModel* m,int flags,s_fileBundle* b,unsigned slot,void* anim,Pose* p,u8* color,void* hot){
 assert(o==&owner&&!flags&&b==&bundle&&slot==0x1E8&&!anim&&p==&pose&&color==reinterpret_cast<u8*>(1)&&!hot);
 m->m18_drawFunction=drawModel;return initSucceeds;
}
void stepAnimation(s_3dModel*){++steps;}
void pushCurrentMatrix(){++depth;calls.push_back(1);}void popMatrix(){assert(depth>0);--depth;calls.push_back(2);}
void translateCurrentMatrix(const sVec3_FP& p){calls.push_back(10+p[0].v);}
void LCSItemBox_DrawType0Sub0(s_fileBundle* b,s16 h,s16 p){assert(b==&bundle);calls.push_back(h);calls.push_back(p);}
void addObjectToDrawList(Model* m){assert(m==&model);calls.push_back(3);}
int sGunShotTask_UpdateSub4(sAnimatedQuad* q){assert(q->frames&&q->frames->size()==1);++ticks;return 0;}
int lagiDrawPoolParticle(sAnimatedQuad*,sVec3_FP* p,fixedPoint scale,fixedPoint bias){
 assert(p->at(0).v==31&&p->at(1).v==42&&p->at(2).v==53);assert(scale.v==0x20000&&bias.v==-0x3000);calls.push_back(4);return 1;
}
#include "lagi/world_map_destinations_draw.h"
#include "production.inc"
int main(){
 using namespace lagi::worldmap;
 assert(!initialize_map_destination_models({},&owner));
 assert(!initialize_map_destination_models({1},nullptr));
 hierarchyPresent=false;assert(!initialize_map_destination_models({1},&owner));hierarchyPresent=true;
 posePresent=false;assert(!initialize_map_destination_models({1},&owner));posePresent=true;
 initSucceeds=false;assert(!initialize_map_destination_models({1},&owner));initSucceeds=true;
 auto* state=initialize_map_destination_models({1},&owner);assert(state&&steps==1&&state->flags==16);
 records[0]={-1,0,{10,20,30}};records[1]={-1,1,{20,30,40}};
 records[2]={-1,2,{25,35,45}};records[3]={-1,3,{30,40,50}};
 assert(draw_map_destination_models(state,{1},{0x10000},4,0,&owner));
 assert(ticks==1&&!depth);
 assert((calls==std::vector<int>{1,20,101,2,1,30,100,2,1,35,3,2,1,4,40,11,0x1E4,0x5C0,2}));
 for(int i=0;i<20;++i)assert(draw_map_destination_models(state,{1},{0x10000},2,0,&owner));
 assert(state->progress[0]==12&&state->progress[1]==0);
 records[0].gate=0; // locked records do not decay their saved progress
 draw_map_destination_models(state,{1},{0x10000},2,1,&owner);
 assert(state->progress[0]==12&&state->progress[1]==1);
 mainGameState.bitField[0]=0x80;
 draw_map_destination_models(state,{1},{0x10000},2,1,&owner);
 assert(state->progress[0]==11&&state->progress[1]==2);
 const auto oldTicks=ticks;
 assert(!draw_map_destination_models(state,{1},{0x10000},51,0,&owner)&&ticks==oldTicks);
 state->flags=0;draw_map_destination_models(state,{1},{0x10000},4,0,&owner);assert(ticks==oldTicks);
 // Exhaust independent story conditions used by native auxiliary passes.
 for(unsigned bits=0;bits<128;++bits){
  mainGameState.bitField={};auto& b=mainGameState.bitField;
  b[7]=(bits&1)?2:0;b[11]=(bits&2)?1:0;b[8]=(bits&4)?0x40:0;
  b[9]=(bits&8)?0x40:0;b[12]=(bits&16)?0x20:0;
  state->flags=0x90;refresh_destination_flags(*state);
  const unsigned expected=0x90|(((bits&1)&&!(bits&2))?4:0)|
   (((bits&4)&&!(bits&8))?1:0)|(((bits&8)&&!(bits&16))?2:0)|((bits&4)?8:0);
  assert(state->flags==expected);
 }
 delete_map_destination_models(state);delete_map_destination_models(nullptr);
 puts("PASS production WORLD destination resources, four type paths, source order, gates, progress, sprite scale/sort bias, flags and ownership; hardware output unverified");
}
