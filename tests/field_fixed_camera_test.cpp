#include <array>
#include <cassert>
#include <cstdio>
using s32=int;
struct fixedPoint {int value=0;fixedPoint()=default;fixedPoint(int v):value(v){}int getInteger()const{return value>>16;}operator int()const{return value;}};
struct sVec3_FP {
 std::array<fixedPoint,3> values{};
 fixedPoint& operator[](int i){return values[i];}const fixedPoint& operator[](int i)const{return values[i];}
 sVec3_FP operator-(const sVec3_FP& p)const{sVec3_FP v;for(int i=0;i<3;++i)v[i]=int(values[i])-int(p[i]);return v;}
};using sVec2_FP=std::array<fixedPoint,2>;
struct CameraScript {sVec3_FP m24_pos2;int m30_thresholdDistance;};
struct s_dragonTaskWorkArea {sVec3_FP m8_pos;CameraScript* m1D0_cameraScript=nullptr;};
static s_dragonTaskWorkArea dragon;
struct SubField {s_dragonTaskWorkArea* m338_pDragonTask;};static SubField sub{&dragon};
struct Field {SubField* m8_pSubFieldData;};static Field field{&sub};Field* getFieldTaskPtr(){return &field;}
struct sFieldCameraStatus {sVec3_FP m0_position,mC_rotation;int m24_distanceToDestination=0,m8D_reinitMode=0,m8F_followType=-1,m90_followMode=-1;};
static sVec3_FP lastDelta,from,to;static int distanceResult=25;
void computeLookAt(const sVec3_FP& delta,sVec2_FP& out){lastDelta=delta;out[0]=delta[1];out[1]=int(delta[0])-int(delta[2]);}
int vecDistance(const sVec3_FP& a,const sVec3_FP& b){from=a;to=b;return distanceResult;}
fixedPoint getSin(int){return 0;}fixedPoint getCos(int){return 0x10000;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return int((static_cast<long long>(int(a))*int(b))>>16);}
fixedPoint MTH_Mul_5_6(fixedPoint a,fixedPoint b,fixedPoint c){return MTH_Mul(a,MTH_Mul(b,c));}
// PRODUCTION_CAMERA
bool same(const sVec3_FP& a,const sVec3_FP& b){for(int i=0;i<3;++i)if(a[i]!=b[i])return false;return true;}
int main() {
 unsigned checks=0;
 for(int mode:{0,1,2,9})for(int frame=0;frame<90;++frame) {
  sFieldCameraStatus camera;camera.m8D_reinitMode=mode;
  for(int i=0;i<3;++i){camera.m0_position[i]=1000+i*200;dragon.m8_pos[i]=2000-frame*(i+1)*3;}
  camera.mC_rotation[2]=0x1234567;const auto original=camera.m0_position;
  const auto expected=dragon.m8_pos-original;
  cameraFollowMode_scriptTarget(&camera);
  assert(same(camera.m0_position,original)&&same(lastDelta,expected));
  assert(camera.mC_rotation[0]==expected[1]);
  assert(camera.mC_rotation[1]==int(expected[0])-int(expected[2]));
  assert(camera.mC_rotation[2]==0x1234567&&camera.m24_distanceToDestination==distanceResult);
  assert(same(from,original)&&same(to,dragon.m8_pos));
  if(mode==0||mode==1)assert(camera.m8D_reinitMode==2&&camera.m8F_followType==0&&camera.m90_followMode==0);
  ++checks;
 }
 // Preserve the established scripted-target branch and its distance clamp.
 CameraScript scripted;scripted.m24_pos2[0]=40;scripted.m30_thresholdDistance=100;
 dragon.m1D0_cameraScript=&scripted;sFieldCameraStatus camera;
 cameraFollowMode_scriptTarget(&camera);
 assert(same(camera.m0_position,scripted.m24_pos2)&&camera.m24_distanceToDestination==25);
 distanceResult=150;cameraFollowMode_scriptTarget(&camera);
 assert(camera.m24_distanceToDestination==100);
 std::printf("PASS production fixed camera: %u live-dragon frames across init/blend/active modes, fixed position, preserved roll, exact look-at arguments and retained scripted clamp\n",checks);
}
