// Compile the generated production dispatcher, not a second state machine.
#include <cassert>
#include <cstdint>
#include <cstdio>
using s8=std::int8_t;
struct fixedPoint {
 int value; fixedPoint(int v=0):value(v){} operator int()const{return value;}
 int toInteger()const{return value;}
};
struct Vec {
 int v[3]{}; int& operator[](int n){return v[n];}
 const int& operator[](int n)const{return v[n];}
 void zeroize(){for(auto& n:v)n=0;}
 Vec& operator+=(const Vec& r){for(int i=0;i<3;++i)v[i]+=r[i];return *this;}
 Vec& operator-=(const Vec& r){for(int i=0;i<3;++i)v[i]-=r[i];return *this;}
};
using sVec3_FP=Vec;
Vec operator-(Vec a,const Vec& b){return a-=b;}
fixedPoint MTH_Mul(int a,int b){return int((std::int64_t(a)*b)>>16);}
Vec MTH_Mul(Vec a,int b){for(int i=0;i<3;++i)a[i]=MTH_Mul(a[i],b);return a;}
int getSin(int){return 0;} int getCos(int){return 65536;}
int randomNumber(){return 0;}
struct Dragon {Vec m8_position;int m8C=0;} dragon;
struct Overlay {Dragon* m18_dragon=&dragon;} overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;} manager,*gBattleManager=&manager;
struct sArachnothFormation {
 Vec m2F0_eatDragonVelocity,m2FC_eatDragonAcceleration,m308_eatDragonPosition;
 Vec m230,m2CC,m224_translation,m254,m26C_rotation,target;
 Vec* m240=&target;
 int m2C4_currentAttackDelay=30,m2C0_currentAttackState=0,m278_targetedQuadrant=0;
};
struct {int m_4D=0,m_4C=0,m0_fade0=0;} g_fadeControls;
struct Reg {int m112_CLOFSL=0;};
struct {Reg m20_registers[2];} vdp2Controls;
int animations=0,damage=0,damageFrame=0,frame=0,fades=0;
void arachnoth_setEatDragonAnim(sArachnothFormation*,int){++animations;}
void playSystemSoundEffect(int){}
void fadePalette(int*,int,int,int){++fades;}
void applyDamageToDragon(int,int amount,Vec,int,Vec,int){damage+=amount;damageFrame=frame;}
void arachnoth_spawnSmokePuff(sArachnothFormation*,Vec*,Vec*,Vec*){}
void battleEngine_setCurrentCameraPositionPointer(Vec*){}
void battleEngine_setDesiredCameraPositionPointer(Vec*){}
void battleEngine_resetCameraInterpolation(){}
#include "../src/integration/lagi_arachnoth_devour.inc"
#include "devour_dispatch.inc"
int main(){
 sArachnothFormation a;
 for(frame=1;frame<=215;++frame){
  arachnoth_eatDragonAttack_update(&a);
  if(frame<30){assert(a.m2C4_currentAttackDelay==30-frame);assert(animations==0);}
  if(frame==30){assert(animations==1);assert(a.m2C0_currentAttackState==1);assert(a.m2C4_currentAttackDelay==0);}
 }
 assert(animations==1&&damage==0x4E&&damageFrame==144&&fades==2);
 assert(a.m2C0_currentAttackState==5);
 puts("PASS generated Devour dispatcher: countdown, animation, both fades, damage once, release within native attack budget; not device acceptance");
}
