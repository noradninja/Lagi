#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
using u8=std::uint8_t;using s8=std::int8_t;using u32=std::uint32_t;using s32=std::int32_t;using s16=std::int16_t;
struct fixedPoint {
 s32 v=0;fixedPoint(s32 n=0):v(n){}s32 asS32()const{return v;}operator s32()const{return v;}
 fixedPoint& operator+=(s32 n){v+=n;return *this;}fixedPoint& operator-=(s32 n){v-=n;return *this;}
};
struct sVec3_FP {
 std::array<fixedPoint,3> v{};
 fixedPoint& operator[](int i){return v[i];}const fixedPoint& operator[](int i)const{return v[i];}
 void zeroize(){v={};}
};
sVec3_FP operator-(sVec3_FP a,const sVec3_FP& b){for(int i=0;i<3;++i)a[i]-=b[i];return a;}
using sVec3_S16=std::array<s16,3>;
struct s_3dModel {s16 m14=0;void (*m18_drawFunction)(s_3dModel*)=nullptr;};
struct sProcessed3dModel {} trailModel;
struct s_fileBundle {sProcessed3dModel* get3DModel(int slot){assert(slot==0x3DC);return &trailModel;}} bundle;
struct s_workAreaCopy {s_fileBundle* m0_fileBundle=&bundle;};
std::vector<int> trailPositions;int pushes=0,pops=0,trailDraws=0;
void pushCurrentMatrix(){++pushes;}void popMatrix(){++pops;}
void translateCurrentMatrix(sVec3_FP p){trailPositions.push_back(p[0]);}
void rotateCurrentMatrixY(fixedPoint){}
void addObjectToDrawList(sProcessed3dModel* p){assert(p==&trailModel);++trailDraws;}
struct sSaturnPtr {int offset=0;bool isNull()const{return !offset;}sSaturnPtr operator+(int n)const{return {offset+n};}};
std::vector<u8> bytes(0x7000);
u8 readSaturnU8(sSaturnPtr p){return bytes.at(p.offset);}s8 readSaturnS8(sSaturnPtr p){return s8(readSaturnU8(p));}
sVec3_FP readSaturnVec3(sSaturnPtr p){sVec3_FP v;for(int i=0;i<3;++i){u32 raw=0;for(int b=0;b<4;++b)raw=(raw<<8)|readSaturnU8(p+i*4+b);v[i]=s32(raw);}return v;}
void put(int ea,s32 value){for(int b=0;b<4;++b)bytes.at(ea+b)=u32(value)>>(24-b*8);}
void putVec(int ea,s32 x,s32 y,s32 z){put(ea,x);put(ea+4,y);put(ea+8,z);}
fixedPoint MTH_Mul(s32 a,s32 b){return s32((std::int64_t(a)*b)>>16);}
fixedPoint MTH_Mul_5_6(s32 a,s32 b,s32 c){return MTH_Mul(MTH_Mul(a,b),c);}
s32 getSin(int){return 0;}s32 getCos(int){return 65536;}
s32 sqrt_F(s32 v){return s32(std::sqrt(double(std::max(v,0))*65536.));}
s32 atan2_FP(s32,s32){return 0;} // camera-angle arithmetic is a native service
s32 vecDistance(const sVec3_FP& a,const sVec3_FP& b){double d=0;for(int i=0;i<3;++i){double n=a[i].v-b[i].v;d+=n*n;}return s32(std::sqrt(d));}
struct sMatrix4x3 {sVec3_FP translation;};
void initMatrixToIdentity(sMatrix4x3* m){m->translation.zeroize();}
void translateMatrix(sVec3_FP p,sMatrix4x3* m){m->translation=p;}
void rotateMatrixY(s32,sMatrix4x3*){}
void transformAndAddVec(sVec3_FP p,sVec3_FP& out,const sMatrix4x3& m){out=p;for(int i=0;i<3;++i)out[i]+=m.translation[i];}
void transformVecByCurrentMatrix(sVec3_FP p,sVec3_FP& out){out=p;}
struct Camera {} cameraProperties2;int cameraCalls=0,lightCalls=0,falloffCalls=0;
void updateEngineCamera(Camera*,sVec3_FP,sVec3_S16){++cameraCalls;}
void setupLight(fixedPoint,fixedPoint,fixedPoint,u32){++lightCalls;}
void generateLightFalloffMap(u32,u32,u32){++falloffCalls;}
#include "lagi/world_map_display.h"
#include "lagi/world_map_motion.h"
#include "production.inc"
int main(){
 using namespace lagi::worldmap;
 MapDisplay s;MapDisplayInput in;in.program={1};in.targetTable={0x6000};in.targetCount=2;
 in.arrivalReady=true;in.arrivalPosition.v={10000,20000,30000};
 putVec(0x6008,10000,20000,30000);putVec(0x6014,40000,50000,60000);
 putVec(0x602c,50000,20000,30000);putVec(0x6038,80000,50000,60000);
 putVec(1+0x4750,0,100000,200000);putVec(1+0x475c,1,2,3);
 for(int i=0;i<12;++i){bytes[1+0x4774+i]=u8(10+i);bytes[1+0x4780+i]=u8(80+i);}
 assert(update_map_display(s,in));assert(s.state==1&&s.previousState==1&&s.selectedTarget==0);
 assert(s.marker.position[0]==10000&&s.poseCounter==12&&s.marker.scale>0&&!(s.marker.flags&0x17));
 const auto before=s.state;in.cameraSelection=100;s.state=0;
 assert(!update_map_display(s,in));assert(s.state==0);in.cameraSelection=0;s.state=before;
 s.selectedTarget=1;s.state=4;assert(update_map_display(s,in));
 assert(s.state==1&&(s.marker.flags&5)==5);
 for(int n=0;n<500;++n)assert(update_map_display(s,in));
 assert(s.marker.position[0]==50000&&!(s.marker.flags&0x17));
 assert(s.marker.scale==65536&&s.marker.poseControl==12);
 assert(s.trailHead>=0&&s.trailHead<16&&s.trailCount==0);
 s.marker.flags|=0x40;
 for(int n=0;n<40;++n)assert(update_map_display(s,in));
 assert(s.colorBlend==31&&s.marker.lightColors[0]==(80u|(81u<<8)|(82u<<16)));
 s.marker.flags&=~0x40;for(int n=0;n<40;++n)assert(update_map_display(s,in));
 assert(s.colorBlend==0&&s.marker.lightColors[0]==(10u|(11u<<8)|(12u<<16)));
 s.state=2;assert(update_map_display(s,in));s.state=3;assert(update_map_display(s,in));
 assert(s.desiredTarget[1]==20000-0x5000);
 assert(s.desiredTarget[0]==10000); // arrival child, not current selection 50000
 in.arrivalReady=false;assert(!update_map_display(s,in));in.arrivalReady=true;
 s.state=5;assert(update_map_display(s,in));assert(s.state==6);
 assert(s.desiredTarget[0]==1&&s.desiredTarget[1]==2&&s.desiredTarget[2]==3);
 for(int n=0;n<30;++n)assert(update_map_display(s,in));
 in.arrivalBusy=true;s.state=1;const s32 scale=s.marker.scale;
 assert(update_map_display(s,in));assert(s.marker.scale<scale);
 apply_map_camera_and_light(s);assert(cameraCalls==1&&lightCalls==1&&falloffCalls==1);
 in.targetCount=1;s.selectedTarget=1;assert(!update_map_display(s,in));
 s_workAreaCopy owner;s.trailHead=1;s.trailCount=3;
 s.trail[1].position[0]=101;s.trail[0].position[0]=100;s.trail[15].position[0]=115;
 assert(draw_map_trail(s,&owner));
 assert(pushes==3&&pops==3&&trailDraws==3&&trailPositions==std::vector<int>({101,100,115}));
 s.trailCount=16;assert(!draw_map_trail(s,&owner));s.trailCount=1;assert(!draw_map_trail(s,nullptr));
 puts("PASS production WORLD display states, target motion, camera transitions, bounded trails, native RGB blend, arrival shrink and camera/light service calls; not a playable-map test");
}
