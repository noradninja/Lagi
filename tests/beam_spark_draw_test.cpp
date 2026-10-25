#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
using s16=std::int16_t;using s32=std::int32_t;using s64=std::int64_t;
#include "../extern/Azel/AzelLib/fixedPoint.h"
struct sVec3_FP {
 std::array<fixedPoint,3> data{};
 sVec3_FP()=default;sVec3_FP(fixedPoint x,fixedPoint y,fixedPoint z):data{x,y,z}{}
 fixedPoint& operator[](unsigned i){return data[i];}
 fixedPoint operator[](unsigned i)const{return data[i];}
 bool operator==(const sVec3_FP& other)const{return data==other.data;}
};
struct sMatrix4x3 {fixedPoint m[3][4];};
#include "spark_native_math.inc"
struct Task {bool finished=false;void markFinished(){finished=true;}};
struct Work {Task task;Task* getTask(){return &task;}};using p_workArea=Work*;
template<class T,class Arg>struct s_workAreaTemplateWithArg:Work {
 using Fn=void(*)(T*);Fn m_UpdateMethod=nullptr,m_DrawMethod=nullptr;
 struct TypedTaskDefinition {void(*init)(T*,Arg);Fn update,draw,destroy;};
};
template<class T,class Arg>T* createSiblingTaskWithArg(p_workArea,Arg arg,const typename T::TypedTaskDefinition* definition){
 auto* result=new T{};result->m_UpdateMethod=definition->update;result->m_DrawMethod=definition->draw;
 definition->init(result,arg);return result;
}
struct Emission {std::array<sVec3_FP,4> points;u16 color;s32 depth;};
static std::vector<Emission> emitted;
void lagiDisplayWorldPolygon(const sVec3_FP* points,std::uint16_t color,std::int32_t depth){
 Emission e{{points[0],points[1],points[2],points[3]},color,depth};emitted.push_back(e);
}
#include "spark_header.inc"
#include "spark_service.inc"
static s32 mulOracle(s32 a,s32 b){return s32((s64(a)*b)>>16);}
static sVec3_FP pointOracle(const sBeamSparkArg& a,unsigned segment,bool top,bool rotated){
 const unsigned angle=(segment%16)*256;
 const s32 local[3]={mulOracle(s32(a.m1C),CosSinTable[angle+1024]),top?s32(a.m20):0,
     mulOracle(s32(a.m1C),CosSinTable[angle])};
 s32 result[3]={local[0],local[1],local[2]};
 if(rotated){
  const unsigned x=(a.mC>>16)&4095,y=(a.m10>>16)&4095;
  const s32 sx=CosSinTable[x],cx=CosSinTable[x+1024],sy=CosSinTable[y],cy=CosSinTable[y+1024];
  // Independent coefficient transcription of native identity -> Y -> X.
  const s32 rows[3][3]={{cy,mulOracle(sy,sx),mulOracle(sy,cx)},
      {0,cx,-sx},{-sy,mulOracle(cy,sx),mulOracle(cy,cx)}};
  for(unsigned r=0;r<3;++r){s64 mac=0;for(unsigned c=0;c<3;++c)mac+=s64(rows[r][c])*local[c];result[r]=s32(mac>>16);}
 }
 sVec3_FP out;for(unsigned axis=0;axis<3;++axis)out[axis]=s32(u32(result[axis])+u32(a.m0_pos[axis].asS32()));
 return out;
}
static auto snapshot(const sBeamSpark& s){return std::array<u32,12>{u32(s.m0_pos[0].asS32()),u32(s.m0_pos[1].asS32()),u32(s.m0_pos[2].asS32()),
 s.mC,s.m10,s.m14,s.m18,s.m1C,s.m20,s.m24,u32(s.m28),u32(s.m2C)|(u32(s.m2E)<<16)};}
static void verify(sBeamSparkArg arg,bool rotated){
 sBeamSpark spark;spark.m_UpdateMethod=beamSpark_Update;spark.m_DrawMethod=beamSpark_Draw;
 beamSpark_Init(&spark,&arg);const auto before=snapshot(spark);
 const auto update=spark.m_UpdateMethod,draw=spark.m_DrawMethod;
 emitted.clear();if(rotated)beamSpark_Draw(&spark);else beamSpark_UpdateAlt(&spark);
 assert(emitted.size()==16 && snapshot(spark)==before && !spark.task.finished);
 assert(spark.m_UpdateMethod==update && spark.m_DrawMethod==draw);
 for(unsigned segment=1;segment<=16;++segment){
  const sVec3_FP previousBottom=pointOracle(arg,segment-1,false,rotated),previousTop=pointOracle(arg,segment-1,true,rotated);
  const sVec3_FP currentBottom=pointOracle(arg,segment,false,rotated),currentTop=pointOracle(arg,segment,true,rotated);
  const std::array<sVec3_FP,4> expected=(segment&1)?
      std::array<sVec3_FP,4>{previousBottom,currentBottom,currentTop,previousTop}:
      std::array<sVec3_FP,4>{currentBottom,previousBottom,previousTop,currentTop};
  assert(emitted[segment-1].points==expected);
  assert(emitted[segment-1].color==arg.m2C && emitted[segment-1].depth==-1);
 }
 // Final segment closes exactly onto the angle-zero edge, including rotation.
 assert(emitted.back().points[0]==pointOracle(arg,0,false,rotated));
 assert(emitted.back().points[3]==pointOracle(arg,0,true,rotated));
}
int main(){
 emitted.reserve(16);
 sBeamSparkArg arg{};arg.m0_pos={12345,-23456,34567};arg.m18=20;
 arg.m1C=0x800;arg.m20=0x14CC;arg.m24=0x4000;arg.m28=-614;arg.m2C=0xFFFF;arg.m2E=0x8421;
 verify(arg,false);verify(arg,true);
 // Every native angular-table index on both axes, with fractional bits that
 // must not affect lookup and high angle bits that must wrap at 4096.
 for(unsigned angle=0;angle<4096;++angle){
  arg.mC=0x1234;arg.m10=(angle<<16)|0xFEDC;verify(arg,true);
  arg.mC=(angle<<16)|0x5678;arg.m10=0xA0000000;verify(arg,true);
 }
 u32 random=0x739A0137;
 for(unsigned i=0;i<1024;++i){
  const auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
  arg.mC=next();arg.m10=next();arg.m14=next();arg.m1C=next();arg.m20=next();arg.m2C=u16(next());
  for(unsigned axis=0;axis<3;++axis)arg.m0_pos[axis]=s32(next());
  verify(arg,true);verify(arg,false);
 }
 arg={};arg.m18=20;arg.m1C=0x800;arg.m20=0x14CC;arg.m24=0x4000;arg.m28=-614;arg.m2C=0xFFFF;arg.m2E=0x8421;
 sBeamSpark spark;spark.m_UpdateMethod=beamSpark_Update;spark.m_DrawMethod=beamSpark_Draw;beamSpark_Init(&spark,&arg);
 assert(spark.m_DrawMethod==beamSpark_UpdateAlt && spark.m_UpdateMethod==beamSpark_Update);
 for(unsigned frame=0;frame<20;++frame){
  emitted.clear();spark.m_DrawMethod(&spark);assert(emitted.size()==16 && emitted[0].color==spark.m2C);
  spark.m_UpdateMethod(&spark);assert(spark.task.finished==(frame==19));
 }
 puts("PASS production spark draws: all 4096 angle indices on both axes, 1024 wrapped cross-axis/radius/height/position cases, exact alternating 16-quad geometry/color/depth, closed seam, untouched state and lifecycle; polygon emitter mocked, device pending");
}
