#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>
#include <utility>
#include <cstdio>
#include <algorithm>
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
struct fixedPoint {
 int32_t value=0; fixedPoint(int32_t v=0):value(v){} int32_t asS32() const{return value;}
 fixedPoint& operator+=(fixedPoint v){value=uint32_t(value)+uint32_t(v.value);return *this;}
 fixedPoint& operator-=(fixedPoint v){value=uint32_t(value)-uint32_t(v.value);return *this;}
};
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return int32_t((int64_t(a.value)*b.value)>>16);}
struct sVec3_FP {fixedPoint m0_X,m4_Y,m8_Z;fixedPoint& operator[](unsigned i){return i==0?m0_X:i==1?m4_Y:m8_Z;} sVec3_FP& operator+=(const sVec3_FP& v){m0_X+=v.m0_X;m4_Y+=v.m4_Y;m8_Z+=v.m8_Z;return *this;}};
using sVec3_S16_12_4=std::array<s16,3>;
struct sSaturnMemoryFile {std::vector<u8> bytes;};
struct sSaturnPtr {u32 m_offset=0;sSaturnMemoryFile* m_file=nullptr;sSaturnPtr operator+(unsigned n)const{return {m_offset+n,m_file};}};
struct File:sSaturnMemoryFile{sSaturnPtr getSaturnPtr(u32 address){return {address,this};}} original,other;
File* gFLD_A5=&original;
s16 readSaturnS16(sSaturnPtr p){auto n=p.m_offset-0x6054000;return s16((p.m_file->bytes.at(n)<<8)|p.m_file->bytes.at(n+1));}
u32 read32(sSaturnPtr p){return (u32(u16(readSaturnS16(p)))<<16)|u16(readSaturnS16(p+2));}
fixedPoint readSaturnFP(sSaturnPtr p){return int32_t(read32(p));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {read32(p),p.m_file};}
sVec3_FP readVec(sSaturnPtr p){return {readSaturnFP(p),readSaturnFP(p+4),readSaturnFP(p+8)};}
struct s_workArea {struct Task{bool finished=false;void markFinished(){finished=true;}}task;Task* getTask(){return &task;} virtual ~s_workArea()=default;s_workArea* parent=nullptr;};
template<class T,class A>struct s_workAreaTemplateWithArg:s_workArea {struct TypedTaskDefinition{void(*init)(T*,A);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
std::vector<s_workArea*> created;
template<class T,class A> T* createSubTaskWithArg(s_workArea* parent,A arg,const typename T::TypedTaskDefinition* def){auto* t=new T;t->parent=parent;created.push_back(t);def->init(t,arg);return t;}
struct s_fileBundle{} bundle;
struct s_memoryAreaOutput{s_fileBundle* m0_mainMemoryBundle=nullptr;u32 m4_characterArea=0;};
std::vector<int> memoryBindings;
void getMemoryArea(s_memoryAreaOutput* m,unsigned slot){m->m0_mainMemoryBundle=&bundle;m->m4_characterArea=0x25C10000;memoryBindings.push_back(slot);}
struct s_DataTable2Sub0{sSaturnPtr m0_function;sVec3_FP m4_position;sVec3_S16_12_4 m10_rotation;s32 m18;sSaturnPtr m1C_modelData;};
struct s_visdibilityCellTask:s_workArea{};
struct s_DataTable3{} environment;
#include "lagi/native_field_cells.h"
struct s_DataTable2{std::vector<std::pair<int,s_DataTable2Sub0>> placements;int area;};
s_DataTable2 objectTable; s_visdibilityCellTask cells[4]; int setupCalls=0,startCalls=0,dispatched=0;
s_DataTable2* readDataTable2(sSaturnPtr p){
 assert(p.m_offset==0x608E34C&&p.m_file==&original);objectTable.placements.clear();
 auto grid=readSaturnEA(p);objectTable.area=read32(p+4);auto n=read32(p+8)*read32(p+12);
 for(u32 i=0;i<n;++i){auto q=readSaturnEA(grid+i*4);while(q.m_offset&&read32(q)){
  s_DataTable2Sub0 row{};row.m0_function=readSaturnEA(q);row.m4_position=readVec(q+4);
  for(int j=0;j<3;++j)row.m10_rotation[j]=readSaturnS16(q+16+j*2);
  row.m18=read32(q+24);row.m1C_modelData=readSaturnEA(q+28);objectTable.placements.push_back({i,row});q=q+32;}}
 return &objectTable;
}
void setupFieldWithCellObjects(s_DataTable3* env,s_DataTable2* objects,void(*start)(s_workArea*)){
 assert(env==&environment);++setupCalls;start(&cells[0]);
 for(auto& [cell,row]:objects->placements){assert(lagiDispatchNativeFieldCellFactory(&cells[cell],row,objects->area));++dispatched;}
}
struct sFieldModelRenderContext{void* m0_parentWorkArea=nullptr;void* m4_initCallback=nullptr;sVec3_FP* m8_pPosition=nullptr;u16 m10_flags=0;u8 m18_visibilityFlags=0;};
void initFieldModelRenderContext(sFieldModelRenderContext* c,void* p,void* callback,sVec3_FP* position,void*,u16 flags,s16,s16,u8,u8){c->m0_parentWorkArea=p;c->m4_initCallback=callback;c->m8_pPosition=position;c->m10_flags=flags;}
int lcsUpdates=0;void updateFieldModelRenderContext(sFieldModelRenderContext*){++lcsUpdates;}
struct {std::array<u8,512> bitField{};}mainGameState;
u32 rng=1;int rngCalls=0;
u32 randomNumber(){++rngCalls;rng=rng*1664525u+1013904223u;return rng&0x7FFFFFFF;}
s32 performModulo2(s32 divisor,s32 dividend){return dividend%divisor;}
bool screen=true;int clipped=0;
void transformAndAddVecByCurrentMatrix(sVec3_FP* p,sVec3_FP* out){*out=*p;}
bool isPointOnScreen(sVec3_FP*,s32 far){assert(far==0x100000);return screen;}
s32 checkPositionVisibilityAgainstFarPlane(sVec3_FP*){return clipped;}
int matrixDepth=0;void pushCurrentMatrix(){++matrixDepth;}void popMatrix(){--matrixDepth;}
void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixZYX_s16(const sVec3_S16_12_4&){}
std::vector<std::pair<int,int>> draws,collisions;
void LCSItemBox_DrawType0Sub0(s_fileBundle* b,s16 model,s16 pose){assert(b==&bundle);draws.push_back({model,pose});}
void LCSItemBox_UpdateType0Sub0(s_fileBundle* b,s32 model,s32 pose,fixedPoint scale){assert(b==&bundle&&scale.value==0x10000);collisions.push_back({model,pose});}
int scripts=0,battles=0;bool scriptSucceeds=true;
int startFieldScript(int index,int bit){assert(index==0&&bit==0x5B5);++scripts;return scriptSucceeds;}
void startBaldorQueenBattle(int encounter,int bit){assert(encounter==0x10&&bit==0x90);++battles;}
struct sVdp1Quad{u8 m0_isLast,m1;};
struct sAnimatedQuad{const std::vector<sVdp1Quad>* m0_quad=nullptr;u16 m4_vdp1Memory=0;u8 m6=0,m7_currentFrame=0;};
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr p){std::vector<sVdp1Quad> q;for(;;p=p+28){auto n=p.m_offset-0x6054000;q.push_back({p.m_file->bytes.at(n),p.m_file->bytes.at(n+1)});if(q.back().m0_isLast)break;}return q;}
void particleInitSub(sAnimatedQuad* p,u16 vram,const std::vector<sVdp1Quad>* q){p->m0_quad=q;p->m4_vdp1Memory=vram;p->m6=p->m7_currentFrame=0;}
int sGunShotTask_UpdateSub4(sAnimatedQuad* p){const auto& q=p->m0_quad->at(p->m7_currentFrame);if(q.m1<=p->m6++){p->m6=0;if(q.m0_isLast)p->m7_currentFrame=0;else++p->m7_currentFrame;}return 0;}
int windCalls=0,particleDraws=0;
void lagiApplyA5WindToParticle(sVec3_FP*){++windCalls;}
int lagiDrawPoolParticle(sAnimatedQuad*,sVec3_FP*,fixedPoint,fixedPoint bias){assert(bias.value==-0x6000);++particleDraws;return 1;}
#include "registry.inc"
#include "production.inc"
void start(s_workArea*){++startCalls;}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream f(argv[1],std::ios::binary);original.bytes={std::istreambuf_iterator<char>(f),{}};assert(!original.bytes.empty());
 lagiSetupGarilMoundField(&environment,start);
 assert(setupCalls==1&&startCalls==1&&dispatched==5&&created.size()==5);
 int enabled=0;Mound* interactive=nullptr;
 for(auto* task:created){auto* a=dynamic_cast<Mound*>(task);assert(a&&a->parent==&cells[1]&&a->memory.m0_mainMemoryBundle==&bundle);
  assert(a->targetPosition.m4_Y.value==0x10000&&a->target.m4_initCallback);
  if(!a->target.m18_visibilityFlags){++enabled;interactive=a;assert(a->kind==2);}
  a->timer=100;Mound::Draw(a);assert(!a->task.finished);
  // Transient draw uses the authored frame pair, with one collision submission.
  a->moving=1;a->frame=38;const auto cursor=readSaturnEA(original.getSaturnPtr(0x608D474+a->kind*4))+38*4;
  Mound::Draw(a);assert(draws.back()==std::make_pair(int(readSaturnS16(cursor)),int(readSaturnS16(cursor+2))));
  a->moving=0;a->frame=0;

 }
 assert(enabled==1&&draws.size()==10&&collisions.size()==10&&matrixDepth==0);
 assert(collisions.back()==std::make_pair(0x1FC,0x730));
 auto* a=interactive;Mound::Target(a);assert(scripts==1&&battles==0);scriptSucceeds=false;Mound::Target(a);assert(scripts==2&&battles==1);
 auto foreign=objectTable.placements[0].second;foreign.m0_function.m_file=&other;assert(!lagiDispatchNativeFieldCellFactory(&cells[0],foreign,2));
 foreign.m0_function.m_file=&original;foreign.m0_function.m_offset+=2;assert(!lagiDispatchNativeFieldCellFactory(&cells[0],foreign,2));
 a->timer=1;screen=false;Mound::Update(a);assert(!a->moving&&a->timer>=240&&a->timer<840);
 a->timer=1;screen=true;auto initial=created.size();Mound::Update(a);assert(a->moving==1&&a->frame==1&&a->cycles==3);
 for(int tick=1;tick<117;++tick)Mound::Update(a);
 assert(!a->moving&&a->frame==0&&a->cycles==0&&created.size()==initial+3);
 assert(a->timer>=240&&a->timer<840);
 auto* sand=dynamic_cast<MoundSand*>(created.back());assert(sand&&sand->parent==a&&sand->initialScale.value==0x6666);
 for(int tick=0;tick<80&&!sand->task.finished;++tick){MoundSand::Update(sand);MoundSand::Draw(sand);}
 assert(sand->task.finished&&sand->replacements==0&&windCalls>0&&particleDraws>0);
 mainGameState.bitField[0x12]=0x80;Mound::Update(a);assert(a->task.finished);
 // Re-entry replaces the binding and spawns exactly five, without stale duplicates.
 const auto before=created.size();lagiSetupGarilMoundField(&environment,start);assert(created.size()==before+5);
 for(auto* task:created)delete task;
 puts("PASS original five placements, owner-keyed dispatch, one interactive mound, script/battle fallback, three native animation/burst cycles, particle retirement and field re-entry");
}
