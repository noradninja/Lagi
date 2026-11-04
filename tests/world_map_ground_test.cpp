#include <cassert>
#include <cstdint>
#include <vector>
#include <utility>
#include <cstdio>
#include <array>
using s32=std::int32_t;using s16=std::int16_t;
struct sSaturnPtr {int offset=0;bool isNull()const{return !offset;}sSaturnPtr operator+(int v)const{return {offset+v};}};
int count=3;bool missingList=false;
sSaturnPtr readSaturnEA(sSaturnPtr p){
 if(p.offset==0x4b25)return {0x6000};
 if(p.offset==0x4b29)return {0x6100};
 if(p.offset==0x6000)return {0x6200};
 assert(p.offset==0x481d);return {missingList?0:0x5000};
}
unsigned readSaturnU8(sSaturnPtr p){
 if(p.offset==0x6004)return 3;
 assert(p.offset>=0x6200&&p.offset<0x6203);
 return std::array<unsigned,3>{2,0,1}[p.offset-0x6200];
}
s32 readSaturnS32(sSaturnPtr p){assert(p.offset==0x4821);return count;}
s16 readSaturnS16(sSaturnPtr p){return s16((p.offset-0x5000)*4+0x20);}
struct Bundle {} bundle;
struct s_workAreaCopy {Bundle* m0_fileBundle=&bundle;} owner;
int pushes=0,pops=0;
void pushCurrentMatrix(){++pushes;}void popMatrix(){++pops;}
using sVec3_FP=std::array<int,3>;
void translateCurrentMatrix(sVec3_FP p){assert((p==sVec3_FP{0,0x5000,0}));}
struct {std::array<std::uint8_t,32> bitField{};} mainGameState;
#include "lagi/world_map_ground.h"
#include "lagi/world_map_panels.h"
std::vector<std::pair<s16,s16>> draws;
void LCSItemBox_DrawType0Sub0(Bundle* b,s16 h,s16 p){assert(b==&bundle);draws.emplace_back(h,p);}
#include "production.inc"
int main(){
 using lagi::worldmap::draw_map_ground_base;
 assert(!draw_map_ground_base({},&owner));
 assert(!draw_map_ground_base({1},nullptr));
 owner.m0_fileBundle=nullptr;assert(!draw_map_ground_base({1},&owner));owner.m0_fileBundle=&bundle;
 missingList=true;assert(!draw_map_ground_base({1},&owner));missingList=false;
 count=-1;assert(!draw_map_ground_base({1},&owner));count=3;
 assert(draw_map_ground_base({1},&owner));
 assert(pushes==1&&pops==1&&draws.size()==3);
 for(int i=0;i<3;++i)assert((draws[i]==std::pair<s16,s16>{s16(0x20+i*16),s16(0x28+i*16)}));
 lagi::worldmap::MapGround state;
 draws.clear();assert(lagi::worldmap::draw_map_ground(state,{1},&owner)&&draws.size()==3);
 state.selected=0;mainGameState.bitField[5]=1;
 for(unsigned sequenceIndex:{2u,0u,1u,1u}) {
  draws.clear();assert(lagi::worldmap::draw_map_ground(state,{1},&owner));
  assert(draws.size()==4);
  assert(draws.back().first==s16((0x6100+sequenceIndex*4-0x5000)*4+0x20));
 }
 assert(state.frame==2&&pushes==pops);
 puts("PASS WORLD ground source-order hierarchy/pose submissions, inherited matrix isolation and invalid-owner guards; hierarchy renderer mocked");
}
