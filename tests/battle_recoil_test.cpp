#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int32_t;
using fixedPoint=s32;
using sVec3_FP=std::array<s32,3>;
s32 MTH_Mul(s32 a,s32 b){return s32((int64_t(a)*b)>>16);}
unsigned draws=0;
unsigned randomNumber(){constexpr unsigned values[]={0x12340000,0xABCD8000,0xFFFFFFFF};assert(draws<3);return values[draws++];}
struct Params {s32 m30_hitTranslation=0x8000,m34_hitRotation=0x4000;};
struct Actor {Params* mCC;sVec3_FP m2C_positionDelta{1,2,3},m50_rotationDelta{4,5,6},m14_positionCurrent{7,8,9};};
void recoil(Actor* pThis,sVec3_FP* knockbackDir){
#include "recoil.inc"
}
int main(){
    Params params;Actor actor{&params};sVec3_FP direction{65536,-65536,32768};
    recoil(&actor,&direction);
    assert((actor.m2C_positionDelta==sVec3_FP{32769,-32766,16387}));
    assert((actor.m50_rotationDelta==sVec3_FP{4,8197,16389}));
    assert((actor.m14_positionCurrent==sVec3_FP{7,8,9}));
    assert(draws==3);
    std::puts("Native authored hit impulse destinations and RNG order PASS");
}
