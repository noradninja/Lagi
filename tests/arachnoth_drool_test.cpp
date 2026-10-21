#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u16=std::uint16_t;
std::uint32_t rng=0; int rngCalls=0;
std::uint32_t randomNumber(){++rngCalls;return rng;}
int MTH_Mul(std::uint32_t a,int b){return int((std::int64_t(a)*b)>>16);}
struct sVec3_FP:std::array<int,3>{
 void zeroize(){fill(0);}
 sVec3_FP& operator+=(const sVec3_FP& b){for(int i=0;i<3;++i)(*this)[i]+=b[i];return *this;}
};
struct quadColor {u16 c[4];};
struct Allocation {u16 m4_vdp1Memory=0x3100;} allocation;
struct s_workAreaCopy{Allocation* m4_vd1Allocation=nullptr;};
struct Owner:s_workAreaCopy{} owner;
struct {Owner* mC_fileBundle=nullptr;}dramAllocatorEnd[1];
struct sArachnothFormation:s_workAreaCopy{};
struct sSmokePuffTask:s_workAreaCopy {
 struct TypedTaskDefinition{void(*init)(sSmokePuffTask*);void(*update)(sSmokePuffTask*);void(*draw)(sSmokePuffTask*);void(*remove)(sSmokePuffTask*);};
 sVec3_FP *m8_positionPtr{},*mC_rotationPtr{};
 sVec3_FP m10_velocity{},m1C_velocity2{},m28_position{1,2,3},m34_acceleration{},m40_velocity3{},m4C_targetPos{4,5,6},m58_offset{};
 int m64_state=0,m68_lifetime=0;
};
void smokePuff_update(sSmokePuffTask*){}
sSmokePuffTask* spawned=nullptr;
template<class T>T* createSubTaskWithCopy(s_workAreaCopy* parent,const typename T::TypedTaskDefinition*){
 assert(parent&&!parent->m4_vd1Allocation);return spawned=new T;
}
struct sMatrix4x3{};
void initMatrixToIdentity(sMatrix4x3*){}
void rotateMatrixShiftedY(int,sMatrix4x3*){}void rotateMatrixShiftedX(int,sMatrix4x3*){}void rotateMatrixShiftedZ(int,sMatrix4x3*){}
void transformAndAddVec(const sVec3_FP& in,sVec3_FP& out,const sMatrix4x3&){out=in;}
bool fromFactory=false;
int draws=0;
void displayRaySegment(std::array<sVec3_FP,2>& points,int width,u16 address,int size,u16 color,const quadColor* gouraud,int mode){
 ++draws;assert(points[0]==sVec3_FP({1,2,3})&&points[1]==(fromFactory?sVec3_FP({1,2,3}):sVec3_FP({4,5,6})));
 assert(width==0x400&&address==u16(0x3100+0xCD0)&&size==0x510&&color==u16(0x3100+0x2EEC)&&mode==0x108);
 for(auto c:gouraud->c)assert(c==0x4210);
}
#include "production.inc"
#include "factory.inc"
int main(){
 for(auto value:{0u,0x80000000u,0xFFFF0000u}){
  rng=value;rngCalls=0;auto offset=lagiArachnothDroolOffset();
  assert(rngCalls==3);
  assert(offset[0]>=-0x3000&&offset[0]<0x3000);
  assert(offset[1]>=-0x1000&&offset[1]<0x1000);
  assert(offset[2]>=0x2000&&offset[2]<0x5000);
  if(value==0x80000000u)assert(offset==sVec3_FP({0,0,0x3800}));
 }
 owner.m4_vd1Allocation=&allocation;dramAllocatorEnd[0].mC_fileBundle=&owner;
 sSmokePuffTask task;assert(!task.m4_vd1Allocation);
 lagiInitArachnothStrandAllocation(&task);smokePuff_draw(&task);assert(draws==1);
 sArachnothFormation formation;sVec3_FP position{1,2,3},rotation{},offset{};
 arachnoth_spawnSmokePuff(&formation,&position,&rotation,&offset);
 assert(spawned&&spawned->m4_vd1Allocation==&allocation&&!formation.m4_vd1Allocation);
 assert(spawned->m8_positionPtr==&position&&spawned->mC_rotationPtr==&rotation);
 fromFactory=true;smokePuff_draw(spawned);assert(draws==2);delete spawned;
 puts("PASS production strand factory with allocation-less parent, native slot-0 ownership, endpoints, texture/palette, stipple and neutral colors; not device acceptance");
}
