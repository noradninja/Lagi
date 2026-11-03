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
s16 readSaturnS16(sSaturnPtr p){return s16((unsigned(readSaturnU8(p))<<8)|readSaturnU8(p+1));}
sVec3_FP readSaturnVec3(sSaturnPtr p){sVec3_FP v;for(int i=0;i<3;++i){u32 raw=0;for(int b=0;b<4;++b)raw=(raw<<8)|readSaturnU8(p+i*4+b);v[i]=s32(raw);}return v;}
void put(int ea,s32 value){for(int b=0;b<4;++b)bytes.at(ea+b)=u32(value)>>(24-b*8);}
void putVec(int ea,s32 x,s32 y,s32 z){put(ea,x);put(ea+4,y);put(ea+8,z);}
fixedPoint MTH_Mul(s32 a,s32 b){return s32((std::int64_t(a)*b)>>16);}
fixedPoint MTH_Mul_5_6(s32 a,s32 b,s32 c){return MTH_Mul(MTH_Mul(a,b),c);}
s32 getSin(int){return 0;}s32 getCos(int){return 65536;}
s32 sqrt_F(s32 v){return s32(std::sqrt(double(std::max(v,0))*65536.));}
struct {
 u8 bitField[64]{};unsigned packed[256]{};
 unsigned readPackedBits(unsigned index,unsigned count){assert(count==6);return packed[index];}
 void setPackedBits(unsigned index,unsigned count,unsigned value){assert(count==6);packed[index]=value;}
} mainGameState;
constexpr u8 bitMasks[]={128,64,32,16,8,4,2,1};
s32 FP_Div(s32 n,s32 d){return s32((std::int64_t(n)*65536)/d);}
s32 atanX=0,atanZ=0;
s32 atan2_FP(s32 x,s32 z){atanX=x;atanZ=z;return 0;} // native service mocked
int headingCalls=0;
fixedPoint interpolateRotation(fixedPoint,fixedPoint target,fixedPoint gain,fixedPoint maximum,s32 minimum){
 assert(gain==0x1999&&maximum==0x38E38E&&minimum==0x12345);++headingCalls;return target;
}
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
 MapDisplayInput destinations;
 assert(initialize_map_destinations(destinations,{1},false,17));
 assert(destinations.targetCount==33&&destinations.cameraSelection==17);
 assert(destinations.targetTable.offset==1+0x5688);
 assert(!initialize_map_destinations(destinations,{1},false,33));
 assert(destinations.cameraSelection==17);
 assert(initialize_map_destinations(destinations,{1},true,0x17));
 assert(destinations.targetCount==4&&destinations.cameraSelection==2);
 assert(destinations.targetTable.offset==1+0x5B2C);
 MapDisplay s;MapDisplayInput in;in.program={1};in.targetTable={0x6000};in.targetCount=2;
 in.arrivalReady=true;in.arrivalPosition.v={10000,20000,30000};
 putVec(0x6008,10000,20000,30000);putVec(0x6014,40000,50000,60000);
 putVec(0x602c,50000,20000,30000);putVec(0x6038,80000,50000,60000);
 assert(initialize_map_arrival(in,1));
 assert(in.arrivalReady&&!in.arrivalBusy);
 assert(in.arrivalPosition[0]==50000&&in.arrivalPosition[1]==20000+0xA000&&in.arrivalPosition[2]==30000);
 assert(!initialize_map_arrival(in,2));
 s32 forwardStep=0,verticalStep=0;
 fixedPoint arrivalYaw;s32 arrivalDistance=0,arrivalHeight=0;
 assert(retarget_map_arrival(in,1,arrivalYaw,arrivalDistance,arrivalHeight));
 assert(arrivalDistance==0 && arrivalHeight==20000+0xA000);
 in.arrivalPosition[0]=51000;in.arrivalPosition[2]=28000;
 assert(retarget_map_arrival(in,1,arrivalYaw,arrivalDistance,arrivalHeight));
 assert(atanX==-4000 && atanZ==8000 && arrivalDistance==2236);
 in.arrivalPosition.v={50000,20000+0xA000,30000};
 update_map_arrival_motion(in,0,0xCCC0,20000,forwardStep,verticalStep);
 assert(forwardStep==0x28&&verticalStep==-0x3D);
 assert(in.arrivalPosition[0]==50000&&in.arrivalPosition[1]==20000+0xA000-0x3D);
 assert(in.arrivalPosition[2]==30000+0x28);
 in.arrivalPosition.v={0,0,0};in.arrivalBusy=false;
 forwardStep=0x1000;verticalStep=0;
 update_map_arrival_motion(in,0,0x8000,16,forwardStep,verticalStep);
 assert(forwardStep==0x800 && in.arrivalPosition[2]==0x800);
 assert(in.arrivalPosition[1]==1 && !in.arrivalBusy);
 forwardStep=0;verticalStep=0;
 update_map_arrival_motion(in,0,0x665F,17,forwardStep,verticalStep);
 assert(in.arrivalBusy && forwardStep==0x28 && in.arrivalPosition[2]==0x828);
 assert(in.arrivalPosition[1]==2);
 in.arrivalBusy=false;forwardStep=0;
 update_map_arrival_motion(in,0,0x6660,18,forwardStep,verticalStep);
 assert(!in.arrivalBusy);
 in.arrivalPosition.v={10000,20000,30000};
 assert(find_map_destination(in,0,2)==1);
 bytes[0x6025]=1;
 assert(find_map_destination(in,0,2)==0);
 mainGameState.bitField[0]=bitMasks[1];
 assert(find_map_destination(in,0,2)==1);
 bytes[0x6025]=0;
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
 // Native 06057CE0 table switch is independent of field travel and preserves
 // the ordinary selection in packed DE until the fleet table is left.
 MapDisplay toggle;MapDisplayInput tables;tables.program={1};
 tables.targetTable={1+0x5688};tables.targetCount=33;toggle.selectedTarget=32;
 assert(!toggle_map_destinations(tables,toggle));
 assert(toggle.selectedTarget==32&&tables.targetCount==33&&!mainGameState.bitField[0x1B]);
 mainGameState.bitField[0x1A]=2;
 mainGameState.packed[0x87]=6;
 assert(toggle_map_destinations(tables,toggle));
 assert((mainGameState.bitField[0x1B]&8)&&tables.targetCount==4&&tables.targetTable.offset==1+0x5B2C);
 assert(toggle.selectedTarget==0&&(toggle.marker.flags&0x40));
 assert(mainGameState.packed[0xDE]==32&&mainGameState.packed[0x87]==6);
 toggle.selectedTarget=3;
 assert(toggle_map_destinations(tables,toggle));
 assert(!(mainGameState.bitField[0x1B]&8)&&tables.targetCount==33&&tables.targetTable.offset==1+0x5688);
 assert(toggle.selectedTarget==32&&!(toggle.marker.flags&0x40)&&mainGameState.packed[0x87]==32);
 tables.program={0};assert(!toggle_map_destinations(tables,toggle));
 MapDisplayInput arrival;arrival.targetTable={0x6000};arrival.targetCount=1;
 assert(initialize_map_arrival(arrival,0));
 assert(arrival.arrivalRotation[0]==0&&arrival.arrivalRotation[1]==0x238E38E&&arrival.arrivalRotation[2]==0);
 update_map_arrival_heading(arrival,0x1234567,0x1000);
 assert(headingCalls==1&&arrival.arrivalRotation[1]==0x1234567);
 update_map_arrival_heading(arrival,0x7654321,0xFFF);
 assert(headingCalls==2&&arrival.arrivalRotation[1]==0x1234567);
 arrival.arrivalReady=false;update_map_arrival_heading(arrival,1,0x2000);
 assert(headingCalls==2);
 puts("PASS production WORLD display states, target motion, camera transitions, bounded trails, native RGB blend, arrival shrink and camera/light service calls; not a playable-map test");
}
