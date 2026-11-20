#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
using u32=std::uint32_t;using s32=std::int32_t;
struct fixedPoint {
 s32 v=0;fixedPoint(s32 value=0):v(value){}s32 asS32()const{return v;}int getInteger()const{return v>>16;}
 fixedPoint normalized()const{return s32((u32(v)&0xFFFFFFF)^0x8000000)-0x8000000;}
 operator s32()const{return v;}
};
struct sVec3_FP {std::array<fixedPoint,3> v{};fixedPoint& operator[](int i){return v[i];}const fixedPoint& operator[](int i)const{return v[i];}};
sVec3_FP operator-(const sVec3_FP&a,const sVec3_FP&b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
fixedPoint MTH_Mul(s32 a,fixedPoint b){return s32((std::int64_t(a)*b.v)>>16);}
fixedPoint MTH_Mul_5_6(s32 a,fixedPoint b,fixedPoint c){return MTH_Mul(MTH_Mul(a,b),c);}
fixedPoint getSin(int angle){return s32(std::llround(std::sin(angle*6.283185307179586/4096)*65536));}
fixedPoint getCos(int angle){return getSin((angle+1024)&4095);}
struct File;
struct sSaturnPtr {u32 m_offset;File* m_file;};
struct File {sSaturnPtr getSaturnPtr(u32 ea){return {ea,this};}} a5,a3;
File* gFLD_A5=&a5;
s32 readSaturnS32(sSaturnPtr ea){
 assert(ea.m_file==&a5);
 switch(ea.m_offset){
 case 0x0605C088:return 0x5B5000;case 0x0605C08C:return 0x14000;
 case 0x0605C090:return s32(0xFFDDA000);case 0x0605C094:return 0x8000000;
 case 0x0605C0A0:return 0x480000;case 0x0605C0A4:return s32(0xFF540000);
 default:assert(false);return 0;
 }
}
struct s_dragonTaskWorkArea_48 {};
struct s_dragonTaskWorkArea {sVec3_FP m8_pos,m20_angle;sVec3_FP* mBC=nullptr;void* mB8_lightWingEffect=nullptr;s_dragonTaskWorkArea_48 m48;} dragon;
struct sFieldCameraStatus {
 sVec3_FP m0_position,mC_rotation;fixedPoint m18,m40;s32 m24_distanceToDestination=0;
 sVec3_FP m44_positionSpring,m50_positionImpulse,m5C_rotationSpring,m68_rotationImpulse;
};
struct CameraManager {sFieldCameraStatus m3E4_cameraSlots[2];} cameras;
struct SubField {s_dragonTaskWorkArea* m338_pDragonTask=&dragon;CameraManager* m334=&cameras;} subfield;
struct Field {SubField* m8_pSubFieldData=&subfield;} field;
Field* getFieldTaskPtr(){return &field;}
std::vector<int> order;
void buildDragonRotationMatrix(s_dragonTaskWorkArea_48* matrix,sVec3_FP* angles){assert(matrix==&dragon.m48&&angles==&dragon.m20_angle);order.push_back(1);}
int activeSlot=1;
sFieldCameraStatus* getFieldCameraStatus(){order.push_back(2);return &cameras.m3E4_cameraSlots[activeSlot];}
s32 altitudePitch=0;
void lagiAzelPrepareGarilCamera(sFieldCameraStatus* camera,s_dragonTaskWorkArea* d){
 assert(camera==&cameras.m3E4_cameraSlots[0]&&d==&dragon);order.push_back(3);
 camera->m18=altitudePitch;camera->m40=0x10000;
}
#include "placement.inc"
int main(){
 auto& camera=cameras.m3E4_cameraSlots[0];auto& active=cameras.m3E4_cameraSlots[1];
 for(auto target:{sSaturnPtr{0x0605C020,&a3},sSaturnPtr{0x0605C054,nullptr},sSaturnPtr{0x060728A4,&a5}})
  assert(!lagiAzelTryGarilPlacement(target)&&order.empty());
 gFLD_A5=nullptr;assert(!lagiAzelTryGarilPlacement({0x0605C020,&a5}));gFLD_A5=&a5;
 dragon.mB8_lightWingEffect=&dragon;
 assert(!lagiAzelTryGarilPlacement({0x0605C020,&a5})&&order.empty()&&dragon.m8_pos[0]==0);
 dragon.mB8_lightWingEffect=nullptr;
 std::array<sVec3_FP,4> rider{};dragon.mBC=rider.data();rider[1][0]=111;rider[3][0]=333;
 camera.m44_positionSpring[0]=11;camera.m50_positionImpulse[1]=22;
 camera.m5C_rotationSpring[2]=33;camera.m68_rotationImpulse[0]=44;
 active.m0_position[0]=1234;
 for(u32 target:{0x0605C020u,0x0605C054u}){
  order.clear();camera.mC_rotation[0]=0;camera.mC_rotation[1]=0;altitudePitch=0;
  assert(lagiAzelTryGarilPlacement({target,&a5}));
  assert((order==std::vector<int>{1,2,3}));
  assert(dragon.m8_pos[0]==(target==0x0605C020?0x5B5000:0x480000));
  assert(dragon.m8_pos[1]==0x14000&&dragon.m8_pos[2]==(target==0x0605C020?s32(0xFFDDA000):s32(0xFF540000)));
  assert(dragon.m20_angle[0]==0&&dragon.m20_angle[1]==0x8000000&&dragon.m20_angle[2]==0);
  assert(rider[2][1]==0x8000000&&rider[1][0]==111&&rider[3][0]==333);
  assert(active.mC_rotation[1]==0x8000000&&active.m0_position[0]==1234);
  assert(camera.m24_distanceToDestination==0x10000);
  assert(camera.m0_position[0]==dragon.m8_pos[0]&&camera.m0_position[1]==dragon.m8_pos[1]);
  assert(camera.m0_position[2]==dragon.m8_pos[2]+0x10000);
  assert(camera.m44_positionSpring[0]==11&&camera.m50_positionImpulse[1]==22);
  assert(camera.m5C_rotationSpring[2]==33&&camera.m68_rotationImpulse[0]==44);
 }
 activeSlot=0;camera.mC_rotation[0]=0;altitudePitch=0;
 assert(lagiAzelTryGarilPlacement({0x0605C020,&a5}));
 assert(camera.mC_rotation[1]==0x8000000);
 assert(camera.m0_position[2]==dragon.m8_pos[2]-0x10000);
 dragon.mBC=nullptr;
 for(s32 pitch:{-0x4000000,0x4000000}){
  camera.mC_rotation[0]=pitch;altitudePitch=0x10000000; // normalizes to zero
  assert(lagiAzelTryGarilPlacement({0x0605C020,&a5}));
  assert(camera.m18==0&&camera.mC_rotation[0]==(pitch<0?-0x31C71C7:0x31C71C7));
 }
 puts("PASS A5 authored placement, rider rotation bounds, matrix/camera order, active yaw versus slot0 reset, pitch bounds and untouched springs; native altitude/trig services mocked");
}
