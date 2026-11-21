#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s16=int16_t;using s32=int32_t;using s64=int64_t;using u32=uint32_t;
#include "fixedPoint.h"
fixedPoint intDivide(fixedPoint divisor,fixedPoint dividend){assert(divisor.asS32());return dividend.asS32()/divisor.asS32();}
struct sVec3_FP {
 std::array<fixedPoint,3> v{};
 fixedPoint& operator[](int i){return v[i];}
 const fixedPoint& operator[](int i)const{return v[i];}
 sVec3_FP operator+(const sVec3_FP& p)const {sVec3_FP out;for(int i=0;i<3;++i)out[i]=v[i]+p[i];return out;}
 sVec3_FP operator-(const sVec3_FP& p)const {sVec3_FP out;for(int i=0;i<3;++i)out[i]=v[i]-p[i];return out;}
 sVec3_FP& operator+=(const sVec3_FP& p){for(int i=0;i<3;++i)v[i]+=p[i];return *this;}
};
struct s_scriptData1 {int m0;sVec3_FP m4,m10,m1C,m28;fixedPoint m34,m38;};
struct s_workArea {};using p_workArea=s_workArea*;
template<class T,class Arg>struct s_workAreaTemplateWithArg:s_workArea {
 struct TypedTaskDefinition {void(*init)(T*,Arg);void(*update)(T*);void(*draw)(T*);void(*del)(T*);};
};
#include "field/fieldCutsceneTask2.h"
struct Script {int m5C=0;};
struct SubField {Script* m34C_ptrToE;};
struct Field {SubField* m8_pSubFieldData;};
static Script script;static SubField sub{&script};static Field field{&sub};
Field* getFieldTaskPtr(){return &field;}
struct Camera {std::array<int,3> mC_rotation{11,22,33};};static Camera camera;
Camera* getFieldCameraStatus(){return &camera;}
static unsigned starts=0,ends=0;
void startCutsceneCameraTracking(sVec3_FP*,sVec3_FP*){++starts;}
void endCutsceneTask(s_cutsceneTask2*){++ends;}
// PRODUCTION_CAMERA
int32_t wrap28(int32_t delta) {
 const uint32_t bits=uint32_t(delta)&0x0FFFFFFF;
 return bits<0x08000000?int32_t(bits):int32_t(bits)-0x10000000;
}
int main() {
 unsigned cases=0;
 const int angles[]={0,0x10000,0x1000000,0x7FFFFFF,0x8000000,0xF000000,0xFFFFFFF};
 for(int duration:{1,2,3,7,60})for(int first:angles)for(int last:angles) {
  std::vector<s_scriptData1> nodes(5);
  auto& n=nodes[3];n.m0=duration;n.m34=first;n.m38=last;
  n.m4[0]=30;n.m1C[0]=30+duration*3;
  n.m10[1]=70;n.m28[1]=70+duration*5;
  s_cutsceneTask2 task{};task.m4=&nodes;task.m38=3;
  const auto expected=wrap28(last-first)/duration;
  assert(task.UpdateSub0()==1&&task.m5C==expected);
  // Task mode/index differ from authored angles and must never enter roll math.
  task.m34=1;camera.mC_rotation={11,22,first};
  for(int frame=1;frame<=duration;++frame) {
   s_cutsceneTask2::Update(&task);
   assert(camera.mC_rotation[0]==11&&camera.mC_rotation[1]==22);
   assert(camera.mC_rotation[2]==first+expected*frame);
  }
  assert(task.m34==-1&&task.m60==duration&&task.m38==4);
  assert(task.m8[0]==30+duration*3&&task.m14[1]==70+duration*5);
  // Camera completion retains the shared script barrier.
  const auto before=ends;script.m5C=1;s_cutsceneTask2::Update(&task);assert(ends==before);
  script.m5C=0;s_cutsceneTask2::Update(&task);assert(ends==before+1);
  ++cases;
 }
 std::vector<s_scriptData1> nodes(2);nodes[0].m0=3;
 nodes[0].m10[0]=10;nodes[0].m28[0]=16;
 sVec3_FP anchor;anchor[0]=100;
 s_cutsceneTask2 task{};task.m4=&nodes;task.m24=&anchor;
 const auto before=starts;s_cutsceneTask2::Update(&task);
 assert(starts==before+1&&task.m14[0]==110);
 for(int frame=1;frame<=3;++frame) {
  anchor[0]=100+frame*20;s_cutsceneTask2::Update(&task);
  assert(task.m14[0]==10+frame*2+100+frame*20);
 }
 std::printf("PASS production shared camera: %u roll/duration cases, wrap direction, yaw preservation, live anchor and completion barrier\n",cases);
}
