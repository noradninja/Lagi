#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cstdio>
#include "lagi/garil_single_formation.h"
#include "lagi_formation_actor_signal.h"
using u8=std::uint8_t;using s8=std::int8_t;using s16=std::int16_t;
using u32=std::uint32_t;using s32=std::int32_t;
struct fixedPoint{s32 m_value{};fixedPoint(s32 n=0):m_value(n){} s32 asS32()const{return m_value;}
 fixedPoint& operator+=(fixedPoint b){m_value+=b.m_value;return *this;}};
struct sVec3_FP:std::array<fixedPoint,3>{friend sVec3_FP operator-(const sVec3_FP& a,const sVec3_FP& b){sVec3_FP r;for(int i=0;i<3;++i)r[i]=a[i].m_value-b[i].m_value;return r;}};
fixedPoint vecDistance(const sVec3_FP& a,const sVec3_FP& b){double sum=0;for(int i=0;i<3;++i){double d=double(a[i].m_value)-b[i].m_value;sum+=d*d;}return s32(std::sqrt(sum));}
struct Task{bool finished{};bool isFinished()const{return finished;}};
struct s_workArea{Task task;Task* getTask(){return &task;}};using p_workArea=s_workArea*;
struct sBTL_A5_SingleEnemy:s_workArea{u32 m14_flags{};};
struct sSaturnPtr{u32 address{};};
struct File{sSaturnPtr getSaturnPtr(u32 a){return {a};}}file;
auto* g_BTL_A5=&file;
sVec3_FP readSaturnVec3(sSaturnPtr p){sVec3_FP v;for(int axis=0;axis<3;++axis)v[axis]=s32(p.address-0x060AF90C+axis);return v;}
struct Group{p_workArea* m0_entityArray{};s8 mE_numEntities=6;sSaturnPtr m8_dataTable;};
struct sLagiGarilSingleFormation{
 u8 mF0_flag{},mF1_pad5[3]{},mF4_entityPositions[216]{};
 std::array<std::array<sVec3_FP,6>,3> m_lagiLayouts{};
 unsigned m_lagiLayout{};u8 m_lagiMoveCounter{};bool m_lagiAttackLead{};
 u8 m28_state=3;s16 m24_counter1{};Group mE0_entityGroup;
};
struct Turn{int m8=1,m0=1;};
struct Flags{bool m2000{},m100_attackAnimationFinished{},m400000{};};
struct Engine{int m22C_dragonCurrentQuadrant{};Turn turn;Turn* m3CC=&turn;
 std::array<fixedPoint,4> m45C_perQuadrantDragonSpeed{};
 std::array<s32,4> m364_perQuadrantDragonAltitude{};Flags m188_flags;}engine;
struct Dragon{u32 m1C0_statusModifiers{};}dragon;
struct Overlay{Engine* m4_battleEngine=&engine;Dragon* m18_dragon=&dragon;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;
auto* gBattleManager=&manager;
std::vector<int> moves,cameras,quadrants,names;unsigned restores{},rngCalls{};u32 rng=0;
bool playerTurn{};
u32 randomNumber(){++rngCalls;return rng;}
void battleEngine_initiateEnemyMoveDragon(s8 direction,s16 agility){assert(agility==60);moves.push_back(direction);}
void battleEngine_PlayAttackCamera(int n){cameras.push_back(n);}
void battleEngine_FlagQuadrantBitForAttack(unsigned n){quadrants.push_back(n);}
void battleEngine_displayAttackName(int n,int duration,int flags){assert(duration==35&&flags==0);names.push_back(n);}
bool battleEngine_isPlayerTurnActive(){return playerTurn;}
void battleEngine_restoreCameraAfterEnemyAttack(){++restores;}
// PRODUCTION_CONTROL
static void phases(sLagiGarilSingleFormation* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// PRODUCTION_SELECTION_GATE
// PRODUCTION_PHASES
 default:break;
}}
static void setDead(sLagiGarilSingleFormation& f,unsigned mask){for(unsigned i=0;i<6;++i){s32 dead=(mask>>i)&1;std::memcpy(f.mF4_entityPositions+i*4,&dead,4);}}
struct Fixture{
 sLagiGarilSingleFormation f;sBTL_A5_SingleEnemy actors[6];p_workArea ptrs[6];
 Fixture(){for(unsigned i=0;i<6;++i)ptrs[i]=&actors[i];f.mE0_entityGroup.m0_entityArray=ptrs;engine.turn={1,1};
  engine.m22C_dragonCurrentQuadrant=0;engine.m188_flags={};dragon.m1C0_statusModifiers=0;
  playerTurn=false;moves.clear();cameras.clear();quadrants.clear();names.clear();}
};
int main(){
 // Every native death mask in front, for normal and status-bit-8 policy.
 for(unsigned mask=0;mask<64;++mask)for(bool status:{false,true}){
  Fixture x;setDead(x.f,mask);dragon.m1C0_statusModifiers=status?8:0;
  phases(&x.f);
  const bool leadersAlive=(mask&3)!=3,othersAlive=(mask&60)!=60;
  const bool attack=status?othersAlive:(leadersAlive||othersAlive);
  if(attack){
   assert(x.f.m28_state==5&&cameras==std::vector<int>{1}&&quadrants==std::vector<int>{1});
   const bool chooseLeads=!status&&leadersAlive;
   assert(x.f.m_lagiAttackLead==chooseLeads);
   for(unsigned i=0;i<6;++i){const bool selected=!(mask&(1u<<i))&&((i<2)==chooseLeads);
    assert(x.actors[i].m14_flags==(selected?0x0C000000u:0));assert(!x.actors[i].task.finished);}
  }else{
   assert(x.f.m28_state==3&&engine.turn.m8==0&&engine.turn.m0==0&&cameras.empty());
   if(status)assert(moves==std::vector<int>{1});else assert(moves.empty());
  }
 }
 // Ordinary no-survivor case has no movement, unlike status-bit-8 routing.
 // The loop above's all-dead normal case is covered separately below.
 {
  Fixture x;setDead(x.f,63);phases(&x.f);assert(moves.empty()&&x.f.m28_state==3);
 }
 for(unsigned quadrant:{1u,3u}){
  Fixture x;engine.m22C_dragonCurrentQuadrant=quadrant;phases(&x.f);
  assert(x.f.m28_state==4&&moves==std::vector<int>{quadrant==1?1:2}&&cameras.empty());
 }
 {
  Fixture x;engine.m22C_dragonCurrentQuadrant=2;phases(&x.f);
  assert(x.f.m28_state==4&&x.f.m_lagiMoveCounter==1&&moves.empty());
  x.f.m28_state=3;phases(&x.f);
  assert(x.f.m28_state==4&&x.f.m_lagiMoveCounter==0&&moves==std::vector<int>{3});
 }
 for(unsigned mode=0;mode<4;++mode){
  Fixture x;setDead(x.f,60);dragon.m1C0_statusModifiers=8;x.f.m_lagiMoveCounter=mode;
  phases(&x.f);
  if(mode<3){assert(x.f.m28_state==3&&engine.turn.m8==0);
   if(mode<2)assert(moves==std::vector<int>{int(mode)+1});else assert(moves.empty());}
  else assert(x.f.m28_state==5&&x.f.m_lagiAttackLead&&x.f.m_lagiMoveCounter==0);
 }
 // Entry gates must neither consume the turn nor ready actors.
 for(unsigned gate=0;gate<3;++gate){Fixture x;
  if(gate==0)playerTurn=true;if(gate==1)engine.turn.m8=0;if(gate==2)x.f.mF1_pad5[2]=1;
  phases(&x.f);assert(x.f.m28_state==3&&cameras.empty()&&moves.empty());
  for(auto& actor:x.actors)assert(actor.m14_flags==0);
 }
 {
  Fixture x;x.actors[0].task.finished=x.actors[1].task.finished=true;
  phases(&x.f);assert(!x.ptrs[0]&&!x.ptrs[1]&&x.f.m28_state==3&&cameras.empty());
 }
 // Exact OLD-counter boundaries: 47 charge ticks, 62 recovery ticks, then
 // engine completion rather than premature cleanup or another selected turn.
 for(bool lead:{false,true}){
  Fixture x;setDead(x.f,lead?60:3);phases(&x.f);assert(x.f.m28_state==5);
  phases(&x.f);assert(x.f.m28_state==5&&names.empty());
  engine.m188_flags.m2000=true;phases(&x.f);
  assert(x.f.m28_state==6&&names==std::vector<int>{lead?8:7});
  for(int i=0;i<46;++i){phases(&x.f);assert(x.f.m28_state==6);}
  phases(&x.f);assert(x.f.m28_state==7&&x.f.m24_counter1==0);
  for(unsigned i=0;i<6;++i)assert(x.actors[i].m14_flags==(((i<2)==lead)?0x0A000000u:0));
  for(int i=0;i<61;++i){phases(&x.f);assert(x.f.m28_state==7&&!engine.m188_flags.m100_attackAnimationFinished);}
  phases(&x.f);assert(x.f.m28_state==8&&engine.m188_flags.m100_attackAnimationFinished);
  playerTurn=true;const auto before=restores;phases(&x.f);assert(x.f.m28_state==8&&restores==before);
  playerTurn=false;phases(&x.f);assert(x.f.m28_state==3&&restores==before+1&&engine.turn.m8==0&&engine.turn.m0==0);
 }
 // Native layout cycling, persistent mutable tables and authored quadrant data.
 for(unsigned selector=0;selector<4;++selector){Fixture x;rng=selector;const auto before=rngCalls;
  lagiInitGarilSingleLayouts(&x.f);assert(rngCalls==before+1&&x.f.m_lagiLayout==(selector==0?1:selector==1?2:0));
  assert(x.f.mF1_pad5[0]==(selector<2?selector+1:0));
  assert(engine.m45C_perQuadrantDragonSpeed[2].m_value==(selector==1?0x20000:0x2A000));
  assert(engine.m364_perQuadrantDragonAltitude[3]==(selector==0?0x26000:selector==1?0x2D000:0x27000));
  x.f.m_lagiLayouts[0][0][0]=1234;
  for(int i=0;i<3;++i)lagiSelectGarilSingleLayout(&x.f);
  assert(x.f.m_lagiLayouts[0][0][0].m_value==1234);
 }
 {
  Fixture x;setDead(x.f,62);rng=0x1000; // only leader self-jitter, exactly zero displacement
  x.f.m_lagiLayouts[0][0][0]=0x1A000;x.f.m_lagiLayouts[0][0][2]=-0x15000;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(rngCalls==before+2&&x.f.m_lagiLayouts[0][0][0].m_value==0x19000&&x.f.m_lagiLayouts[0][0][2].m_value==-0x14000);
 }
 // Spacing is order-dependent: the second pair sees the first pair's
 // already-mutated position. Cases cover repel, jitter, attract and clamps.
 {
  Fixture x;setDead(x.f,51);rng=0x1000;
  x.f.m_lagiLayouts[0][3][0]=0x100;x.f.m_lagiLayouts[0][3][2]=0x100;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(rngCalls==before);
  assert(x.f.m_lagiLayouts[0][2][0].m_value==-0x1000&&x.f.m_lagiLayouts[0][2][2].m_value==-0x1000);
  assert(x.f.m_lagiLayouts[0][3][0].m_value==0x1100&&x.f.m_lagiLayouts[0][3][2].m_value==0x1100);
 }
 {
  Fixture x;setDead(x.f,51);rng=0x1000;x.f.m_lagiLayouts[0][3][0]=0x10000;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(rngCalls==before+4&&x.f.m_lagiLayouts[0][2][0].m_value==0&&x.f.m_lagiLayouts[0][3][0].m_value==0x10000);
 }
 {
  Fixture x;setDead(x.f,51);rng=0x1000;x.f.m_lagiLayouts[0][3][0]=0x50000;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(rngCalls==before+2&&x.f.m_lagiLayouts[0][2][0].m_value==0);
  assert(x.f.m_lagiLayouts[0][3][0].m_value==0x19000&&x.f.m_lagiLayouts[0][3][2].m_value==0x4000);
 }
 {
  Fixture x;setDead(x.f,51);rng=0x1000;x.f.m_lagiLayouts[0][3][0]=0xA000;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(rngCalls==before+4&&x.f.m_lagiLayouts[0][3][0].m_value==0xA000);
 }
 {
  Fixture x;setDead(x.f,51);x.actors[3].task.finished=true;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(!x.ptrs[3]&&rngCalls==before);
 }
 {
  Fixture x;setDead(x.f,63);x.f.mF1_pad5[1]=64;x.f.mF1_pad5[0]=1;
  x.f.m_lagiLayouts[0][0][0]=1234;
  const auto before=rngCalls;lagiUpdateGarilSingleSpacing(&x.f);
  assert(rngCalls==before&&x.f.mF1_pad5[1]==0&&x.f.m_lagiLayout==2&&x.f.mF1_pad5[0]==2);
  assert(x.f.m_lagiLayouts[0][0][0].m_value==1234);
 }
 std::puts("Single formation: selection/death masks, camera/movement handoffs, native phase boundaries and writable layout lifetime PASS");
}
