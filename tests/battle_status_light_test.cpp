#include <cassert>
#include <cstdint>
#include <cstdio>
using s8=int8_t; using s32=int32_t; using s64=int64_t; using u32=uint32_t;
struct fixedPoint{
 s32 v=0; fixedPoint()=default; fixedPoint(s32 n):v(n){}
 static fixedPoint fromInteger(s32 n){return n*65536;}
 static fixedPoint fromS32(s64 n){return s32(n);}
 s32 asS32()const{return v;} operator s32()const{return v;}
};
struct sVec3_FP{fixedPoint v[3];fixedPoint& operator[](int n){return v[n];}const fixedPoint& operator[](int n)const{return v[n];}};
struct s_battleGrid{sVec3_FP m1F0;};
struct Overlay{s_battleGrid* m8_gridTask;};struct Manager{Overlay* m10_battleOverlay;};
s_battleGrid grid;Overlay overlay{&grid};Manager manager{&overlay};Manager* gBattleManager=&manager;
struct Data{ s32 words[12];const s32* getSaturnPtr(u32 a){assert(a>=0x060AE38C&&a<0x060AE3BC);return words+(a-0x060AE38C)/4;}} data;
// Native pointer arithmetic is byte-based.
struct Ptr{const s32* p;Ptr operator+(int n)const{assert(n%4==0);return {p+n/4};}};
struct View{Ptr getSaturnPtr(u32 a){return {data.getSaturnPtr(a)};}} view;
View* g_BTL_GenericData=&view;s32 readSaturnS32(Ptr p){return *p.p;}
struct s_RGB8{u32 value;static s_RGB8 fromVector(const sVec3_FP& v){return {u32((v[0].v>>16)&255)|u32((v[1].v>>16)&255)<<8|u32((v[2].v>>16)&255)<<16};}u32 toU32(){return value;}};
u32 output;int calls;void setupLightColor(u32 c){output=c;++calls;}
struct Task{fixedPoint m208;s32 m20C;s32 m210;};
#include "status.inc"
int main(){
 for(int type=0;type<4;++type)for(int channel=0;channel<3;++channel)data.words[type*3+channel]=(-8+type*3+channel);
 grid.m1F0={{fixedPoint::fromInteger(4),fixedPoint::fromInteger(8),fixedPoint::fromInteger(12)}};
 for(int type=0;type<4;++type)for(int reverse=0;reverse<2;++reverse)for(int remaining=0;remaining<=30;++remaining){
  Task t{30*65536,reverse,remaining};calls=0;run(&t,type,1);assert(calls==1);
  u32 expected=0;
  for(int ch=0;ch<3;++ch){
   int effect=data.words[type*3+ch],base=4+ch*4;
   int start=reverse?effect:base,finish=reverse?base:effect;
   s64 raw=s64(start)*65536+s64(finish-start)*65536*(30-remaining)/30;
   expected|=u32((raw>>16)&255)<<(8*ch);
  }
  assert(output==expected);
 }
 Task t{30*65536,0,30};calls=0;run(&t,0,0);assert(calls==0);
 puts("PASS: generated status-light envelope, four entries, both phases, all countdown frames and inactive path. Not hardware validation.");
}
