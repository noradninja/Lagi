#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include "lagi_formation_actor_signal.h"
using u8=std::uint8_t;using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u32=std::uint32_t;
struct Task{bool finished{};bool isFinished()const{return finished;}};
struct Actor{Task task;u32 flags{};Task* getTask(){return &task;}};using p_workArea=Actor*;
struct sSaturnPtr{u32 address{};};struct File{sSaturnPtr getSaturnPtr(u32 n){return {n};}}file;
auto* g_BTL_A5=&file;
struct Group{Actor** m0_entityArray{};int mE_numEntities=20,mF_deadCount{};sSaturnPtr m8_dataTable;};
struct sBTL_A5_FormationTask{Group mE0_entityGroup;int m28_state=3,m1D4_flag{},mF0_flag{},m26_counter2{};u8 mF1_pad5[3]{};s16 m24_counter1{};};
struct Turn{int m8=1,m0=1,m2=1,m4=1;};
struct Flags{bool m2000{},m100_attackAnimationFinished{},m400000{},m20_battleIntroRunning{},m10000{};};
struct Interpolator{int m68_rate{};};
struct s_battleEngine{Turn turn;Turn* m3CC=&turn;Flags m188_flags;int m22C_dragonCurrentQuadrant{},m22E_dragonMoveDirection{},m38E=7,m38C{};
 Interpolator m27C_dragonMovementInterpolator1,m2E8_dragonMovementInterpolator2;}engine;
struct Overlay{s_battleEngine* m4_battleEngine=&engine;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;
auto* gBattleManager=&manager;
enum eBattleModes{m7=7,mA=10};
std::vector<int> moves,modes,quads,sounds,names;unsigned randomCalls{};u32 rng{};bool playerTurn{};
u32 randomNumber(){++randomCalls;return rng;}
void playSystemSoundEffect(s32 n){sounds.push_back(n);}
void battleEngine_initiateEnemyMoveDragon(s8 n,s16 delay){assert(delay==60);moves.push_back(n);}
void battleEngine_SetBattleMode(eBattleModes n){modes.push_back(n);engine.m38C=n;engine.m188_flags.m2000=false;engine.m188_flags.m20_battleIntroRunning=true;}
void battleEngine_FlagQuadrantBitForAttack(u32 n){quads.push_back(n);}
void battleEngine_displayAttackName(int n,int duration,int flags){assert(flags==0);assert((n==1&&duration==30)||(n==2&&duration==35)||(n==3&&duration==40));names.push_back(n);}
bool battleEngine_isPlayerTurnActive(){return playerTurn;}
u32& lagiA5ActorFlags(p_workArea a){return a->flags;}
void lagiA5SignalActorFlags(p_workArea a,u32 n){a->flags|=n;}
// PRODUCTION_EVADE
// PRODUCTION_SCATTER
// PRODUCTION_CONTROL
static void phases(sBTL_A5_FormationTask* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// PRODUCTION_GATE
// PRODUCTION_PHASES
 default:break;
}}
struct Fixture{sBTL_A5_FormationTask f;std::array<Actor,20> actors;std::array<Actor*,20> ptrs;
 Fixture(){for(unsigned i=0;i<20;++i)ptrs[i]=&actors[i];f.mE0_entityGroup.m0_entityArray=ptrs.data();f.mE0_entityGroup.m8_dataTable={0x060B0D24};
  engine=s_battleEngine{};engine.m3CC=&engine.turn;moves.clear();modes.clear();quads.clear();sounds.clear();names.clear();playerTurn=false;randomCalls=0;rng=0;}
};
int main(){
 for(u32 random:{0u,1u}){Fixture x;rng=random;phases(&x.f);
  assert(x.f.m28_state==4&&moves==std::vector<int>{random?2:1}&&randomCalls==1&&sounds.empty()&&modes.empty());
  assert(engine.turn.m8==0&&engine.turn.m0==0);
 }
 for(int quadrant:{1,3})for(int count:{20,10}){
  Fixture x;x.f.mE0_entityGroup.mE_numEntities=count;engine.m22C_dragonCurrentQuadrant=quadrant;
  // Selection counts live actors and clears coordinated tags on all survivors.
  x.actors[0].task.finished=true;x.ptrs[1]=nullptr;
  for(auto& a:x.actors)a.flags=0x08000010u;
  phases(&x.f);assert(x.f.m28_state==5&&!x.ptrs[0]);
  assert(modes==std::vector<int>{7}&&quads==std::vector<int>{quadrant==1?2:8}&&sounds==std::vector<int>{0x68}&&randomCalls==0);
  for(int i=2;i<count;++i)assert(x.actors[i].flags==(i<5?0x04000010u:0x10u));
  assert(x.actors[0].flags==0x08000010u); // finished work area never touched
  phases(&x.f);assert(x.f.m28_state==5&&names.empty());
  engine.m188_flags.m2000=true;phases(&x.f);
  assert(x.f.m28_state==6&&names==std::vector<int>{1});
  for(int i=2;i<5;++i)assert(x.actors[i].flags==0x02000010u);
  for(int i=0;i<16;++i){phases(&x.f);assert(x.f.m28_state==6&&!engine.m188_flags.m100_attackAnimationFinished);}
  phases(&x.f);assert(x.f.m28_state==4&&engine.m188_flags.m100_attackAnimationFinished&&x.f.m24_counter1==0);
 }
 for(int quadrant:{1,3}){Fixture x;engine.m22C_dragonCurrentQuadrant=quadrant;
  for(int turn=0;turn<3;++turn){x.f.m28_state=3;engine.turn={1,1,1,1};phases(&x.f);assert(x.f.m28_state==(turn<2?5:4));}
  assert(x.f.mF1_pad5[0]==0&&moves==std::vector<int>{quadrant==1?2:1}&&modes.size()==2&&sounds.size()==2);
 }
 for(bool variant:{false,true}){Fixture x;engine.m22C_dragonCurrentQuadrant=2;x.f.m1D4_flag=variant;x.f.mE0_entityGroup.mE_numEntities=variant?10:20;
  phases(&x.f);assert(x.f.m28_state==7&&modes==std::vector<int>{10}&&randomCalls==0);
  assert(x.f.mE0_entityGroup.m8_dataTable.address==(variant?0x060B0F90u:0x060B0E14u));
  assert(engine.m22E_dragonMoveDirection==3&&engine.m38E==0&&engine.m27C_dragonMovementInterpolator1.m68_rate==60&&engine.m2E8_dragonMovementInterpolator2.m68_rate==60);
  for(int i=0;i<x.f.mE0_entityGroup.mE_numEntities;++i)assert(x.actors[i].flags==0x0C000000u);
  phases(&x.f);assert(x.f.m28_state==7&&names.empty());engine.m188_flags.m2000=true;phases(&x.f);
  assert(x.f.m28_state==8&&names==std::vector<int>{2}&&quads==std::vector<int>{4});
  for(int i=0;i<x.f.mE0_entityGroup.mE_numEntities;++i)assert(x.actors[i].flags==0x0A000000u);
  for(int i=0;i<61;++i){phases(&x.f);assert(x.f.m28_state==8);}
  phases(&x.f);assert(x.f.m28_state==4&&x.f.mE0_entityGroup.m8_dataTable.address==0x060B0D24&&engine.m188_flags.m100_attackAnimationFinished);
 }
 for(bool active:{false,true}){Fixture x;playerTurn=active;engine.turn.m8=active?1:0;phases(&x.f);
  assert(x.f.m28_state==3&&moves.empty()&&modes.empty()&&randomCalls==0);}
 {Fixture x;engine.m22C_dragonCurrentQuadrant=1;for(auto& a:x.actors)a.task.finished=true;phases(&x.f);
  assert(x.f.m28_state==3&&modes.empty()&&sounds==std::vector<int>{0x68});for(auto* p:x.ptrs)assert(!p);}
 // Queen loss waits behind the existing engine intro/attack barrier. Ready
 // actors are released before Scatter can replace the formation's state.
 {Fixture x;engine.m22C_dragonCurrentQuadrant=1;phases(&x.f);x.f.mF0_flag=1;
  lagiA5BeginQueenScatter(&x.f,&engine);assert(x.f.m28_state==5&&!x.f.m1D4_flag);
  engine.m188_flags.m2000=true;phases(&x.f);assert(x.f.m28_state==6);
  for(int i=0;i<3;++i)assert(x.actors[i].flags==0x02000000u);
  engine.m188_flags.m20_battleIntroRunning=false;lagiA5BeginQueenScatter(&x.f,&engine);
  assert(x.f.m28_state==9&&x.f.m1D4_flag);engine.m188_flags.m2000=true;lagiA5UpdateQueenScatter(&x.f,&engine);
  assert(x.f.m28_state==0xB&&engine.m188_flags.m10000&&engine.m188_flags.m100_attackAnimationFinished);
  for(auto& a:x.actors)assert(a.flags&8);
 }
 std::puts("PASS generated Nanyd formation: movement policy, live actor selection, Valley evade setup, release/complete boundaries, variant tables and queen-loss barrier");
}
