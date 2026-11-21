#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
using u8=uint8_t;using u16=uint16_t;using s16=int16_t;using u32=uint32_t;using s32=int32_t;using s64=int64_t;
#include "fixedPoint.h"
struct sVec3_FP {fixedPoint m0_X,m4_Y,m8_Z;};
struct sSaturnPtr {u32 m_offset=0;sSaturnPtr operator+(u32 n)const{return {m_offset+n};}};
static std::vector<u8> prg;
u32 read(sSaturnPtr p,unsigned count){u32 v=0;const auto o=p.m_offset-0x6054000;assert(o+count<=prg.size());for(unsigned i=0;i<count;++i)v=(v<<8)|prg[o+i];return v;}
s16 readSaturnS16(sSaturnPtr p){return s16(read(p,2));}u16 readSaturnU16(sSaturnPtr p){return u16(read(p,2));}
s32 readSaturnS32(sSaturnPtr p){return s32(read(p,4));}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {readSaturnS32(p),readSaturnS32(p+4),readSaturnS32(p+8)};}
struct Overlay {sSaturnPtr getSaturnPtr(u32 p){return {p};}};static Overlay overlay;static Overlay* gFLD_A5=&overlay;
struct State {std::array<u8,512> bitField{};
// PRODUCTION_BITS
};static State mainGameState;
struct s_workArea {};using p_workArea=s_workArea*;
template<class T,class Arg>struct s_workAreaTemplateWithArg:s_workArea {void(*m_DrawMethod)(T*)=nullptr;};
struct sLCSTarget {};
#include "field/fieldModelRender.h"
struct Hierarchy {int countNumberOfBones(){return 1;}};struct sStaticPoseData {};struct sProcessed3dModel {};struct s_3dModel {};
struct s_fileBundle {
 std::array<u8,1024> raw{};Hierarchy hierarchy;sStaticPoseData pose;sProcessed3dModel model;
 sStaticPoseData* getStaticPose(int,int){return &pose;}Hierarchy* getModelHierarchy(int){return &hierarchy;}
 u8* getRawBuffer(){return raw.data();}sProcessed3dModel* get3DModel(int){return &model;}
};static s_fileBundle bundle;
struct s_memoryAreaOutput {s_fileBundle* m0_mainMemoryBundle=nullptr;};
void getMemoryArea(s_memoryAreaOutput* m,int){m->m0_mainMemoryBundle=&bundle;}
int init3DModelRawData(s_workArea*,s_3dModel*,int,s_fileBundle*,int,void*,sStaticPoseData*,void*,void*){return 1;}
void stepAnimation(s_3dModel*){}
struct sMatrix4x3 {std::array<std::array<fixedPoint,4>,3> m{};};
void initMatrixToIdentity(sMatrix4x3*){}void rotateMatrixShiftedY(fixedPoint,sMatrix4x3*){}
static sVec3_FP lastNormal;
void transformVec(const sVec3_FP& p,sVec3_FP& out,const sMatrix4x3&){lastNormal=p;out=p;}
void initFieldModelRenderContext(sFieldModelRenderContext* ctx,void* owner,void* cb,sVec3_FP* p,sVec3_FP* normal,u16 flags,s16,s16,u8,u8) {
 *ctx={};ctx->m0_parentWorkArea=owner;ctx->m4_initCallback=cb;ctx->m8_pPosition=p;ctx->mC_pNormal=normal;ctx->m10_flags=flags;
}
static unsigned registered=0;
void updateFieldModelRenderContext(sFieldModelRenderContext*){++registered;}
struct sDebrisScatterParams {
 fixedPoint m0_gravity,m4_bounce,m8_spread,mC_randomMask;
 sVec3_FP *m10_pPosition=nullptr,*m14_pRotation=nullptr;
 fixedPoint m18_velX,m1C_velY,m20_velZ;s_fileBundle* m_pBundle=nullptr;
};
static unsigned debris=0;static sVec3_FP debrisPosition,debrisRotation;
void initDebrisScatterConfig(sDebrisScatterParams* p,u16,u16){*p={};}
u32 READ_BE_U32(const u8* p){return (u32(p[0])<<24)|(u32(p[1])<<16)|(u32(p[2])<<8)|p[3];}
s32 readMaxScalarFromBundleTree(u8*,u32){return 0x10000;}
void createDebrisScatterTask(p_workArea,sDebrisScatterParams* p,bool) {
 ++debris;assert(p->m10_pPosition&&p->m14_pRotation);
 debrisPosition=*p->m10_pPosition;debrisRotation=*p->m14_pRotation;
}
void playSystemSoundEffect(s32){}
void pushCurrentMatrix(){}void popMatrix(){}void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixZYX(sVec3_FP*){}
static sMatrix4x3 matrix;static sMatrix4x3* pCurrentMatrix=&matrix;
// Source draw reads only the translation component for LOD.

u32 gridCellDraw_GetDepthRange(fixedPoint){return 0;}
void addObjectToDrawList(sProcessed3dModel*){}
void a5WormPassageEntrance_dispatchCollision(void*,s16){}
void fieldNoop(){}
struct Dragon {
 void(*mF0)(Dragon*)=nullptr;s32 m104_dragonScriptStatus=0;u32 mF8_Flags=0;
 sVec3_FP m8_pos,m20_angle,m160_deltaTranslation;
};using s_dragonTaskWorkArea=Dragon;static Dragon dragon;
struct Sub {Dragon* m338_pDragonTask;};static Sub sub{&dragon};
struct Field {Sub* m8_pSubFieldData;};static Field field{&sub};Field* getFieldTaskPtr(){return &field;}
struct Camera {sVec3_FP m0_position;};using sFieldCameraStatus=Camera;static Camera camera;Camera* getFieldCameraStatus(){return &camera;}
enum {eCameraFollowMode_scriptTarget=1};void setCameraFollowMode_cut(int){}
void DragonUpdateCutscene(Dragon*){}
fixedPoint getSin(unsigned){return 0;}fixedPoint getCos(unsigned){return 0x10000;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((int64_t(a.asS32())*b.asS32())>>16);}
static unsigned transitions=0;static int destination=-100;
void triggerSubfieldChange(int subfield,int arg){assert(arg==-0x8000);++transitions;destination=subfield;}
void fieldA3_1_checkExitsTaskUpdate2Sub1(int arg){assert(arg==0xB);++transitions;destination=-1;}
void dispatchTutorialMultiChoiceSub2(){++transitions;destination=-2;}
void playBattleSoundEffect(int){}
// PRODUCTION_PASSAGE
bool same(const sVec3_FP& a,const sVec3_FP& b){return a.m0_X==b.m0_X&&a.m4_Y==b.m4_Y&&a.m8_Z==b.m8_Z;}
int main(int argc,char** argv) {
 assert(argc==2);std::ifstream in(argv[1],std::ios::binary);prg.assign(std::istreambuf_iterator<char>(in),{});assert(!prg.empty());
 const u32 records[]={0x6098AF0,0x6098B50,0x6098BB0,0x6098C10,0x6098C70,0x6098CD0,0x6098D30};
 for(auto ea:records) {
  mainGameState={};sA5WormPassageEntrance actor{};sSaturnPtr record{ea};
  a5WormPassageEntrance_Init(&actor,record);
  assert(same(actor.lagiPosition,readSaturnVec3(record))&&same(actor.lagiRotation,readSaturnVec3(record+12)));
  assert(actor.mC_modelCtx.m8_pPosition==&actor.lagiPosition&&actor.mC_modelCtx.m4_initCallback);
  if(readSaturnS16(record+0x26))assert(same(lastNormal,readSaturnVec3({0x6098AE4})));
  const int unlock=readSaturnS16(record+0x5E),gate=readSaturnS16(record+0x22);
  if(actor.m9C_config==0&&(unlock||gate)){a5WormPassageEntrance_Update(&actor);assert(actor.m9C_config==0);}
  if(unlock) {
   assert(actor.mA0_state==1&&actor.mA8_modelIdx[0]==readSaturnS16(record+0x18));
   // A neighboring story bit must not unlock the passage.
   mainGameState.setBit566(unlock+1);auto before=debris;
   a5WormPassageEntrance_DrawWithModel(&actor);assert(debris==before&&actor.mA0_state==1);
   mainGameState.setBit566(unlock);a5WormPassageEntrance_DrawWithModel(&actor);
   assert(debris==before+1&&actor.mA0_state==2);
   assert(same(debrisPosition,actor.lagiPosition)&&same(debrisRotation,actor.lagiRotation));
   a5WormPassageEntrance_DrawWithModel(&actor);assert(debris==before+1);
   sA5WormPassageEntrance reloaded{};a5WormPassageEntrance_Init(&reloaded,record);
   assert(reloaded.mA0_state==2&&reloaded.mA8_modelIdx[0]==readSaturnS16(record+0x56));
  }
  if(gate)mainGameState.setBit566(gate);
  if(actor.m9C_config==0)a5WormPassageEntrance_Update(&actor);
  assert(actor.m9C_config==1);const auto registeredBefore=registered;
  a5WormPassageEntrance_Update(&actor);assert(registered==registeredBefore+1);
  auto callback=reinterpret_cast<void(*)(p_workArea,sLCSTarget*)>(actor.mC_modelCtx.m4_initCallback);
  auto expectedBits=mainGameState.bitField;
  const int visit=readSaturnS16(record+0x2A),adjusted=visit<1000?visit:visit-566;
  expectedBits[adjusted/8]|=u8(0x80>>(adjusted%8));
  callback(&actor,nullptr);assert(actor.m9C_config==2&&mainGameState.bitField==expectedBits);
  assert(mainGameState.getBit566(readSaturnS16(record+0x2A)));
  const auto before=transitions;a5WormPassageEntrance_Update(&actor);
  assert(actor.m9C_config==3&&actor.mA4_delay==90&&dragon.mF0==DragonUpdateCutscene);
  for(int i=0;i<89;++i)a5WormPassageEntrance_Update(&actor);
  assert(transitions==before&&actor.m9C_config==3);
  a5WormPassageEntrance_Update(&actor);
  assert(transitions==before+1&&actor.m9C_config==4);
  assert(destination==readSaturnS16(record+0x28));
  a5WormPassageEntrance_Update(&actor);assert(transitions==before+1);
 }
 std::puts("PASS production worm passages: seven original records, decoded stable targets/normals, exact unlock bits, debris transform, LCS selection/visit and 90-frame native transition");
}
