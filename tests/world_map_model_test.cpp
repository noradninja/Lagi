#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using u8=std::uint8_t;using u32=std::uint32_t;using s32=std::int32_t;using s16=std::int16_t;
struct fixedPoint {s32 v=0;fixedPoint(s32 n=0):v(n){} s32 asS32()const{return v;}};
struct sVec3_FP {std::array<int,3> v{};void zeroize(){v={};}};
struct sSaturnPtr {int offset=0;bool isNull()const{return !offset;}sSaturnPtr operator+(int n)const{return {offset+n};}};
u8 readSaturnU8(sSaturnPtr p){return u8(p.offset);}
struct s_3dModel {s16 m14=0;void (*m18_drawFunction)(s_3dModel*)=nullptr;};
bool hierarchyPresent=true,posePresent=true;int steps=0;
struct Hierarchy {int countNumberOfBones(){return 7;}} hierarchy;
struct Pose {} pose;
struct s_fileBundle {
 Hierarchy* getModelHierarchy(int slot){assert(slot==0x1EC);return hierarchyPresent?&hierarchy:nullptr;}
 Pose* getStaticPose(int slot,int bones){assert(slot==0x5C8&&bones==7);return posePresent?&pose:nullptr;}
 u8* getRawFileAtOffset(int slot){assert(slot==0x5D0);return reinterpret_cast<u8*>(1);}
} bundle;
struct s_workAreaCopy {s_fileBundle* m0_fileBundle=&bundle;};
std::vector<int> calls;
void draw(s_3dModel* model){assert(model->m14==-3);calls.push_back(7);}
bool init3DModelRawData(s_workAreaCopy* owner,s_3dModel* model,int flags,s_fileBundle* file,int slot,
 void* animation,Pose* p,u8* colors,void* hotpoints){
 assert(owner&&file==&bundle&&flags==0&&slot==0x1EC&&!animation&&p==&pose&&colors==reinterpret_cast<u8*>(1)&&!hotpoints);
 model->m18_drawFunction=draw;return true;
}
void stepAnimation(s_3dModel*){++steps;}
void pushCurrentMatrix(){calls.push_back(1);}void popMatrix(){calls.push_back(8);}
void translateCurrentMatrix(sVec3_FP p){assert(p.v[0]==10);calls.push_back(2);}
void rotateCurrentMatrixY(fixedPoint p){assert(p.v==20);calls.push_back(3);}
void scaleCurrentMatrixRow0(s32 p){assert(p==30);calls.push_back(4);}
void scaleCurrentMatrixRow1(s32 p){assert(p==30);calls.push_back(5);}
void scaleCurrentMatrixRow2(s32 p){assert(p==30);calls.push_back(6);}
#include "lagi/world_map_model.h"
#include "production.inc"
int main(){
 using namespace lagi::worldmap;
 MapModel state;s_workAreaCopy owner;
 assert(!initialize_map_model(state,nullptr,{1}));
 assert(!initialize_map_model(state,&owner,{}));
 hierarchyPresent=false;assert(!initialize_map_model(state,&owner,{1}));hierarchyPresent=true;
 posePresent=false;assert(!initialize_map_model(state,&owner,{1}));posePresent=true;
 assert(initialize_map_model(state,&owner,{1}));
 assert(steps==1&&state.scale.v==0&&state.yaw.v==0&&state.transitionRate==0x2000&&state.flags==0);
 assert(state.lightAngle==s32(0xFF1C71C8u));
 for(unsigned i=0;i<4;++i){unsigned b=1+i*3;assert(state.lightColors[i]==(b|((b+1)<<8)|((b+2)<<16)));}
 draw_map_model(state);assert(calls.empty());
 state.position.v[0]=10;state.yaw=20;state.scale=30;state.poseControl=-3;state.flags=0x20;
 draw_map_model(state);assert(calls==std::vector<int>({1,2,3,4,5,6,7,8}));
 puts("PASS production WORLD model resources, RGB byte order, visibility, matrix order and pose control; live world map remains unported");
}
