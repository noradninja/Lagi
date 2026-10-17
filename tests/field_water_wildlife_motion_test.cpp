#include <cassert>
#include <cstdint>
#include <cstdio>
using u32=uint32_t;
using s32=int32_t;
using fixedPoint=int32_t;
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((int64_t(a)*b)>>16);}
fixedPoint MTH_Mul_5_6(fixedPoint a,fixedPoint b,fixedPoint c){return MTH_Mul(a,MTH_Mul(b,c));}
// Cardinal native 16.16 trig values; reject unexpected table indices.
fixedPoint getSin(u32 a){assert(a%1024==0); const int t[]={0,65536,0,-65536};return t[(a/1024)%4];}
fixedPoint getCos(u32 a){return getSin((a+1024)%4096);}
struct Vec{fixedPoint m0_X,m4_Y,m8_Z;};
struct Target{int updates=0;};
void updateLCSTarget(Target* t){++t->updates;}
struct Task{
 Vec m8_position{},m14_LCSPosition{};
 fixedPoint m20_rotAngle=0,m24_computedY=0,m28_amplitude=0,m2C_param=0,m30_param2=0;
 Target m40_LCSTarget;
};
#include "motion.inc"
int main(){
 for(int phase=0;phase<4;++phase)for(int heading=0;heading<4;++heading){
  Task t;
  t.m8_position={100<<16,-2*65536,200<<16};
  t.m14_LCSPosition={0,t.m8_position.m4_Y,0};
  t.m20_rotAngle=phase*0x4000000;
  t.m30_param2=heading*0x4000000;
  t.m28_amplitude=4*65536;t.m2C_param=32768;
  const int dx[]={0,4,0,-4},dz[]={2,0,-2,0};
  const int c[]={1,0,-1,0},s[]={0,1,0,-1};
  motion(&t);
  assert(t.m14_LCSPosition.m0_X==(100+dx[phase]*c[heading]-dz[phase]*s[heading])*65536);
  assert(t.m14_LCSPosition.m8_Z==(200+dx[phase]*s[heading]+dz[phase]*c[heading])*65536);
  assert(t.m14_LCSPosition.m4_Y==-2*65536);
  assert(t.m24_computedY==t.m20_rotAngle-t.m30_param2+0x4000000);
  assert(t.m40_LCSTarget.updates==1);
 }
 puts("PASS: generated wildlife orbit updates X/Z, heading and target; Y retained. Not a hardware visual test.");
}
