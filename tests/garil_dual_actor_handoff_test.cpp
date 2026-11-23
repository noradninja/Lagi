#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s16=int16_t;using u16=uint16_t;
using u8=std::uint8_t;using u32=std::uint32_t;using s32=std::int32_t;
struct FP{int32_t value=0;FP()=default;FP(int32_t v):value(v){}int32_t asS32()const{return value;}FP& operator+=(int n){value+=n;return *this;}bool operator==(const FP&)const=default;};
struct sVec3_FP:std::array<FP,3>{};
#include <cstring>
#include "lagi_formation_actor_signal.h"
#include "lagi/native_formation_motion.h"
struct Task{bool finished=false;bool isFinished(){return finished;}};struct s_workArea{Task task;Task* getTask(){return &task;}};struct s_workAreaCopy:s_workArea{};using p_workArea=s_workArea*;
struct sSaturnPtr{u32 address;};
struct File{sSaturnPtr getSaturnPtr(u32 x){return {x};}}file;
auto* g_BTL_A5=&file;
struct Target{sVec3_FP* m8;};
struct Model{Target* m54_targetables;unsigned m52_targetableCount;};
struct Formation{u8 mF0_flag=0;u8 mF1_pad5[3]{};};
struct sBTL_A5_SingleEnemy:s_workAreaCopy{
    std::vector<Model> m2C_models;
    int m1BC_isLead2{};u8 m28_state{};u32 m14_flags{};
    Formation* m158_formation;
    struct{uint8_t* activeMask=nullptr;uint8_t* reactionMask=nullptr;}m_lagiGroup;
};
struct Turn{int m8=1,m0=1;}turn;
struct Engine{sVec3_FP m1A0_battleAutoScrollDelta;Turn* m3CC=&turn;struct{bool m2000=true,m100_attackAnimationFinished=false;}m188_flags;};
struct Dragon{sVec3_FP mF0;}dragon;
struct Overlay{Engine* m4_battleEngine;Dragon* m18_dragon=&dragon;};struct Manager{Overlay* m10_battleOverlay;};
Engine engine;Overlay overlay{&engine};Manager manager{&overlay};auto* gBattleManager=&manager;
struct Call{bool charge;s_workAreaCopy* parent;const sVec3_FP* sourcePtr;const u8* state;
    u8 active;sVec3_FP source,velocity;sSaturnPtr quad;u8 slot;int damage,flags;};
std::vector<Call> calls;std::vector<int> sounds;
p_workArea lagiCreateNativeBattleChargeQuad(s_workAreaCopy* parent,const sVec3_FP* p,
    const u8* state,u8 active,sSaturnPtr q,u8 slot){
    assert(*state==1);calls.push_back({true,parent,p,state,active,{}, {},q,slot,0,0});return nullptr;
}
p_workArea lagiCreateNativeHomingBattleProjectile(s_workAreaCopy* parent,
    const sVec3_FP& source,const sVec3_FP& velocity,sSaturnPtr q,u8 slot,int damage,int flags){
    calls.push_back({false,parent,nullptr,nullptr,0,source,velocity,q,slot,damage,flags});return nullptr;
}
unsigned randomCalls{},animationSteps{},positionUpdates{};
u32 randomNumber(){++randomCalls;return 5;}
s32 performModulo2(u32 divisor,u32 value){assert(divisor==3);return value%divisor;}
void stepAnimation(Model*){++animationSteps;}
void BTL_A5_singleEnemy_updatePositionFromFormation(sBTL_A5_SingleEnemy*){++positionUpdates;}
void playSystemSoundEffect(int n){sounds.push_back(n);}
// PRODUCERS
static void updateAttackStages(sBTL_A5_SingleEnemy* pThis)
{
    switch(pThis->m28_state){
// ACTOR_STAGES
    default:break;
    }
}

struct Group{int mE_numEntities=3;p_workArea* m0_entityArray;};
struct Dual{Group m1D0_entityGroup2;unsigned m28_state=6;s16 m24_counter1=0;int m280_phase=0;struct{sVec3_FP m0_position;}m2C_posBlock;sVec3_FP mA8_position,mC0_position2;};
u32& lagiGarilSingleActorFlags(p_workArea p){return static_cast<sBTL_A5_SingleEnemy*>(p)->m14_flags;}
int names=0,cameras=0;void battleEngine_displayAttackName(int n,int t,int f){assert(n==8&&t==35&&!f);++names;}
void battleEngine_enableAttackCamera(){++cameras;}void battleEngine_setDesiredCameraPositionPointer(sVec3_FP*){}void battleEngine_setCurrentCameraPositionPointer(sVec3_FP*){}void battleEngine_resetCameraInterpolation(){}
void battleEngine_restoreCameraDefault(){}void battleEngine_restoreCameraAfterEnemyAttack(){}bool battleEngine_isPlayerTurnActive(){return false;}
// TIMER
void updateFormation(Dual* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// FORMATION_PHASES
}}
int main(){
 for(int mask=1;mask<8;++mask){
  Formation binding;std::array<sBTL_A5_SingleEnemy,3> actors;std::array<p_workArea,3> pointers;
  sVec3_FP world[3][3];Target targets[3][3];
  for(int i=0;i<3;++i){pointers[i]=&actors[i];for(int j=0;j<3;++j){world[i][j]={i*100+j,20,30};targets[i][j]={&world[i][j]};}actors[i].m2C_models.push_back({targets[i],3});actors[i].m1BC_isLead2=1;actors[i].m_lagiGroup.activeMask=&binding.mF0_flag;actors[i].m_lagiGroup.reactionMask=&binding.mF1_pad5[1];if(mask&(1<<i))actors[i].m14_flags=0x0C000000;}
  Dual dual;dual.m1D0_entityGroup2={3,pointers.data()};calls.clear();sounds.clear();names=cameras=0;randomCalls=0;
  int count=0;for(int i=0;i<3;++i)if(mask&(1<<i))++count;
  for(int tick=0;tick<80;++tick){
   updateFormation(&dual);for(auto& actor:actors)updateAttackStages(&actor);
   if(tick==0){assert(names==1&&calls.size()==count);for(const auto& c:calls)assert(c.charge);}
   if(tick<47)assert(calls.size()==count);
   if(tick==47){assert(dual.m28_state==8&&cameras==1&&calls.size()==2*count);for(int i=0;i<3;++i)assert(actors[i].m28_state==((mask&(1<<i))?2:0));}
  }
  assert(dual.m28_state==0&&turn.m8==0&&turn.m0==0&&engine.m188_flags.m100_attackAnimationFinished);
  assert(calls.size()==2*count&&randomCalls==unsigned(count));
  for(int i=count;i<2*count;++i)assert(!calls[i].charge&&calls[i].damage==33&&calls[i].flags==8);
 }
 puts("PASS all seven dual secondary selection masks: production formation phases release real actor stages once into lead projectile service");
}
