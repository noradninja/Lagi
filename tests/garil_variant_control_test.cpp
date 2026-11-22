#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "lagi_formation_actor_signal.h"
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
struct s_workAreaCopy{struct Task{bool finished=false;bool isFinished(){return finished;}void markFinished(){finished=true;}}task;Task* getTask(){return &task;}};using p_workArea=s_workAreaCopy*;
struct sBTL_A5_FormationTask;
struct sBTL_A5_VariantEnemy:s_workAreaCopy{u32 m14_flags=0;sBTL_A5_FormationTask* m158_formation=nullptr;int m1C_entityIndex=0;u8 m1D5_wasDamaged=0;};
struct Group{p_workArea* m0_entityArray=nullptr;int mE_numEntities=3;};
struct sBTL_A5_FormationTask:s_workAreaCopy{Group mE0_entityGroup;std::array<u8,3> mF1_pad5{};std::array<u8,8> mF4_entityPositions{};u8 m28_state=3;s16 m24_counter1=0,m26_counter2=0;};
struct Turn{int m8=1,m0=77;}turn;
struct Engine{Turn* m3CC=&turn;int m22C_dragonCurrentQuadrant=0;struct{bool m2000=false,m400000=false,m100_attackAnimationFinished=false;}m188_flags;}engine;
struct Overlay{Engine* m4_battleEngine=&engine;}overlay;struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
enum class eBattleModes{m7};unsigned modes=0,attackMask=0,names=0,releases=0;bool player=false;
void battleEngine_SetBattleMode(eBattleModes m){assert(m==eBattleModes::m7);++modes;}
void battleEngine_FlagQuadrantBitForAttack(u16 mask){attackMask=mask;}
bool battleEngine_isPlayerTurnActive(){return player;}
void battleEngine_displayAttackName(int name,int duration,int flags){assert(name==15&&duration==30&&!flags);++names;}
void decreaseNPCRefCount(int slot){assert(slot==14);++releases;}
u16 radar=0xA500;std::vector<unsigned> radarCalls;
void battleEngine_FlagQuadrantBitForSafety(u16 mask){radarCalls.push_back(mask);if(mask)radar|=mask&15;else radar&=~15;}
void battleEngine_FlagQuadrantBitForDanger(u16 mask){radarCalls.push_back(0x10000u|mask);if(mask)radar|=(mask&15)<<4;else radar&=~0xF0;}
struct File{u32 getSaturnPtr(u32 a){return a;}}file;auto* g_BTL_A5=&file;
u32 readSaturnU32(u32 a){assert(a>=0x60AD194&&a<=0x60AD1A0);return (a-0x60AD194)/4;}
#include "quadrants.inc"
#include "membership.inc"
#include "control.inc"
void phases(sBTL_A5_FormationTask* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
#include "phases.inc"
}}
int main(){
 for(unsigned a=0;a<256;++a)for(unsigned b=0;b<5;++b)for(unsigned c=0;c<5;++c){
  sBTL_A5_FormationTask f;f.mF4_entityPositions[0]=a;f.mF4_entityPositions[1]=b;f.mF4_entityPositions[2]=c;radar=0xA5FF;radarCalls.clear();
  lagiUpdateGarilVariantQuadrants(&f);unsigned safe=0,danger=0;
  for(unsigned q=0;q<4;++q){const unsigned count=(a==q)+(b==q)+(c==q);if(!count)danger|=1u<<q;else if(count>1)safe|=1u<<q;}
  assert(radar==(0xA500|safe|(danger<<4)));assert(radarCalls==std::vector<unsigned>({0,0x10000,safe,0x10000|danger}));
 }
 for(unsigned index=0;index<3;++index)for(unsigned quadrant=0;quadrant<4;++quadrant)for(unsigned slot=0;slot<256;++slot)for(unsigned mask=0;mask<256;++mask){
  sBTL_A5_FormationTask f;sBTL_A5_VariantEnemy actor;actor.m158_formation=&f;actor.m1C_entityIndex=index;f.mF4_entityPositions[index]=slot;f.mF1_pad5[2]=mask;engine.m22C_dragonCurrentQuadrant=quadrant;
  lagiUpdateGarilVariantMembership(&actor);assert(f.mF1_pad5[2]==(slot==quadrant?(mask|(1u<<index)):(mask&~(1u<<index))));
 }
 for(unsigned index=0;index<3;++index)for(unsigned mask=0;mask<256;++mask)for(bool active:{false,true}){
  sBTL_A5_FormationTask f;sBTL_A5_VariantEnemy actor;actor.m158_formation=&f;actor.m1C_entityIndex=index;f.mF1_pad5[1]=mask;f.mF1_pad5[2]=0xA5;
  lagiSetGarilVariantReaction(&actor,active);assert(f.mF1_pad5[1]==(active?(mask|(1u<<index)):(mask&~(1u<<index))));assert(f.mF1_pad5[2]==0xA5);
 }
 for(unsigned byte=0;byte<256;++byte)for(u32 flags:{0u,0xFFFFFFFFu,0x60000000u,0x06000004u}){
  sBTL_A5_VariantEnemy actor;actor.m1D5_wasDamaged=byte;actor.m14_flags=flags;lagiConsumeGarilVariantDamage(&actor);assert(!actor.m1D5_wasDamaged);assert(actor.m14_flags==(byte==1?flags&~0x60000000u:flags));
 }
 unsigned cases=0;
 for(unsigned slots=0;slots<27;++slots)for(unsigned mask=0;mask<8;++mask)for(unsigned old:{0u,1u,255u})for(unsigned q=0;q<4;++q){
  std::array<sBTL_A5_VariantEnemy,3> actors;std::array<p_workArea,3> pointers;unsigned tmp=slots;bool selected=false;
  for(unsigned i=0;i<3;++i){const auto status=tmp%3;tmp/=3;pointers[i]=status?&actors[i]:nullptr;actors[i].task.finished=status==2;if(status==1&&(mask&(1u<<i)))selected=true;actors[i].m14_flags=0x100;}
  sBTL_A5_FormationTask f;f.mE0_entityGroup.m0_entityArray=pointers.data();f.mF1_pad5[2]=mask;f.mF4_entityPositions[4]=old;engine.m22C_dragonCurrentQuadrant=q;turn={1,77};modes=attackMask=0;
  lagiSelectGarilVariantTurn(&f);assert(f.m28_state==(selected?6:4)&&modes==1);assert(attackMask==(selected?1u<<q:0));
  tmp=slots;for(unsigned i=0;i<3;++i){const auto status=tmp%3;tmp/=3;if(status!=1){assert(!pointers[i]);continue;}const auto expected=selected?((mask&(1u<<i))?0x04000000u:0u):(old==0?1u:2u);assert(actors[i].m14_flags==(0x100u|expected));}
  assert(f.mF4_entityPositions[4]==(selected?old:old==0?1:0));++cases;
 }
 std::array<sBTL_A5_VariantEnemy,3> actors;std::array<p_workArea,3> pointers={&actors[0],&actors[1],&actors[2]};
 for(bool active:{false,true})for(unsigned latch:{0u,1u})for(int ready:{0,1,2}){
  sBTL_A5_FormationTask f;f.mE0_entityGroup.m0_entityArray=pointers.data();f.mF1_pad5[1]=latch;turn={ready,77};player=active;f.m28_state=3;phases(&f);assert(f.m28_state==(!active&&!latch&&ready==1?4:3));
 }
 player=false;
 for(unsigned state:{4u,7u,8u})for(int old=-32768;old<=32767;++old){
  sBTL_A5_FormationTask f;f.mE0_entityGroup.m0_entityArray=pointers.data();f.m28_state=state;f.m24_counter1=f.m26_counter2=s16(old);engine.m188_flags={};releases=0;
  phases(&f);const int limit=state==4?15:state==7?105:75;const bool done=old>limit;assert(f.m28_state==(done&&state!=8?5:state));assert(f.task.finished==(state==8&&done));assert(releases==unsigned(state==8&&done));assert(engine.m188_flags.m100_attackAnimationFinished==(state!=8&&done));
  const auto value=state==8?f.m26_counter2:f.m24_counter1;assert(value==(done?0:s16(u16(old+1))));
 }
 for(bool gate:{false,true}){sBTL_A5_FormationTask f;f.mE0_entityGroup.m0_entityArray=pointers.data();f.m28_state=6;actors[0].m14_flags=0x04000010;actors[1].m14_flags=0x80;actors[2].m14_flags=0x04000020;engine.m188_flags.m2000=gate;names=0;phases(&f);assert(f.m28_state==(gate?7:6)&&names==unsigned(gate));assert(actors[0].m14_flags==(gate?0x02000010:0x04000010));assert(actors[1].m14_flags==0x80);}
 for(bool busy:{false,true}){sBTL_A5_FormationTask f;f.mE0_entityGroup.m0_entityArray=pointers.data();f.m28_state=5;turn={1,77};engine.m188_flags.m400000=busy;phases(&f);assert(f.m28_state==(busy?5:3));assert(turn.m8==(busy?1:0)&&turn.m0==(busy?77:0));}
 printf("PASS variant selection %u cases, turn ownership, release masks, all signed counter values and native states 3-8\n",cases);
}
