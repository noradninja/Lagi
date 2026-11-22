#include <array>
#include <vector>
#include <span>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "lagi_formation_actor_signal.h"
#include "lagi_battle_enemy_lifecycle.h"
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
constexpr u32 TASK_FLAGS_FINISHED=1;
struct s_workAreaCopy{struct Task{u32 m14_flags=0;bool isFinished(){return m14_flags&1;}void markFinished(){m14_flags|=1;}}task;Task* getTask(){return &task;}};using p_workArea=s_workAreaCopy*;
struct sVec3_FP{};
struct sBattleTargetable{bool deleted=false;};
struct Animation{u16 flags=0,bones=0,m4_numFrames=60;}animation;
struct Model{Animation* m30_pCurrentAnimation=&animation;u16 m16_previousAnimationFrame=0;unsigned frame=0,m52_targetableCount=2;sBattleTargetable* m54_targetables=nullptr;};
struct File{Animation* getAnimation(u16 slot){assert(slot==0x48||slot==0x4C||slot==0x50);return &animation;}u32 getSaturnPtr(u32 a){return a;}}file;auto* g_BTL_A5=&file;
u32 readSaturnU32(u32 a){assert(a>=0x60AD194&&a<=0x60AD1A0);return (a-0x60AD194)/4;}
struct Group{p_workArea* m0_entityArray=nullptr;int mE_numEntities=3,mF_deadCount=0;};
struct sBTL_A5_FormationTask:s_workAreaCopy{Group mE0_entityGroup;std::array<u8,3> mF1_pad5{};std::array<u8,8> mF4_entityPositions{};struct{sVec3_FP m0_position;}m2C_posBlock;u8 m28_state=3;s16 m24_counter1=0,m26_counter2=0;int m1D4_flag=0;};
struct Meter:s_workAreaCopy{u8 m31=0;};
struct sBTL_A5_VariantEnemy:s_workAreaCopy{u32 m14_flags=0;sBTL_A5_FormationTask* m158_formation=nullptr;int m1C_entityIndex=0;u8 m28_state=0,m1D5_wasDamaged=0;std::vector<Model> m2C_models=std::vector<Model>(1);File* m0_fileBundle=&file;s16 mB2_health=100;Meter* mAC_lifeMeter=nullptr;sVec3_FP m30_position,m3C_rotation;int queuedDamage=0;};
struct Turn{int m8=1,m0=7,m2=9,m4=11;}turn;
struct Engine{Turn* m3CC=&turn;u8 m22C_dragonCurrentQuadrant=0;int m3B2_numBattleFormationRunning=0;struct{bool m2000=true,m100_attackAnimationFinished=false,m400000=false;}m188_flags;sVec3_FP mC_battleCenter;}engine;using s_battleEngine=Engine;
struct Overlay{Engine* m4_battleEngine=&engine;}overlay;struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
struct Slot{void* mC_fileBundle=nullptr;};Slot dramAllocatorEnd[7];
enum class eBattleModes{m7};unsigned fires=0,debris=0,releases=0;
void battleEngine_SetBattleMode(eBattleModes){}void battleEngine_FlagQuadrantBitForAttack(u16){}void battleEngine_FlagQuadrantBitForSafety(u16){}void battleEngine_FlagQuadrantBitForDanger(u16){}
bool battleEngine_isPlayerTurnActive(){return false;}bool battleEngine_isBattleIntroFinished(){return true;}
void battleEngine_displayAttackName(int name,int frames,int flags){assert(name==15&&frames==30&&!flags);}
void battleEngine_setupAttackCamera(sVec3_FP*,s32,u8,s16){}void battleEngine_restoreCameraAfterEnemyAttack(){}
void* lagiCreateNativeBattleSpeech(sBTL_A5_FormationTask*,s16,s16,u32){return nullptr;}
void decreaseNPCRefCount(int slot){assert(slot==14);++releases;}
void formationUpdatePosition(auto*,sVec3_FP*){}
void lagiUpdateGarilVariantPosition(sBTL_A5_VariantEnemy*){}void lagiUpdateGarilVariantRotation(sBTL_A5_VariantEnemy*){}void lagiUpdateGarilVariantDamageValues(sBTL_A5_VariantEnemy*){}
bool lagiUpdateGarilVariantDamage(sBTL_A5_VariantEnemy* a){if(!a->queuedDamage)return false;a->mB2_health-=a->queuedDamage;a->queuedDamage=0;return true;}
void playSystemSoundEffect(int sound){assert(sound==0x65||sound==0x66||sound==0x6D);}
void initAnimation(Model* m,Animation*){m->frame=0;m->m16_previousAnimationFrame=0;}
s16 stepAnimation(Model* m){m->m16_previousAnimationFrame=m->frame;return s16(m->frame);}
void lagiFireGarilVariantBurst(sBTL_A5_VariantEnemy*){++fires;}
void deleteTargetable(sBattleTargetable* t){assert(!t->deleted);t->deleted=true;}
template<class... T>void createBoneDebrisExplosion(T...){++debris;}
// MEMBERSHIP
// CONTROL
// QUADRANTS
// ACTOR
// FORMATION
int main(){
 sBTL_A5_FormationTask formation;std::array<sBTL_A5_VariantEnemy,3> actors;std::array<Meter,3> meters;std::array<std::array<sBattleTargetable,2>,3> targets;std::array<p_workArea,3> pointers;
 for(unsigned i=0;i<3;++i){pointers[i]=&actors[i];actors[i].m158_formation=&formation;actors[i].m1C_entityIndex=i;actors[i].mAC_lifeMeter=&meters[i];actors[i].m2C_models[0].m54_targetables=targets[i].data();formation.mF4_entityPositions[i]=i;}
 formation.mE0_entityGroup.m0_entityArray=pointers.data();
 for(auto& a:actors)BTL_A5_enemyVariant_Update(&a);assert(formation.mF1_pad5[2]==1);
 BTL_A5_formationVariant_Update(&formation);assert(formation.m28_state==6&&actors[0].m14_flags&0x04000000);
 BTL_A5_enemyVariant_Update(&actors[0]);assert(actors[0].m28_state==3);
 BTL_A5_formationVariant_Update(&formation);assert(formation.m28_state==7&&!(actors[0].m14_flags&0x04000000)&&actors[0].m14_flags&0x02000000);
 BTL_A5_enemyVariant_Update(&actors[0]);assert(actors[0].m28_state==4);actors[0].m2C_models[0].frame=46;BTL_A5_enemyVariant_Update(&actors[0]);assert(fires==1&&actors[0].m28_state==5);actors[0].m2C_models[0].frame=59;BTL_A5_enemyVariant_Update(&actors[0]);assert(actors[0].m28_state==0);
 for(unsigned i=0;i<107;++i)BTL_A5_formationVariant_Update(&formation);assert(formation.m28_state==5);BTL_A5_formationVariant_Update(&formation);assert(formation.m28_state==3&&turn.m8==0&&turn.m0==0);
 actors[1].queuedDamage=10;BTL_A5_enemyVariant_Update(&actors[1]);assert(actors[1].m28_state==6&&formation.mF1_pad5[1]==2);turn.m8=1;BTL_A5_formationVariant_Update(&formation);assert(formation.m28_state==3);
 actors[1].m2C_models[0].frame=43;BTL_A5_enemyVariant_Update(&actors[1]);assert(actors[1].m28_state==8&&formation.mF4_entityPositions[1]==0);actors[1].m2C_models[0].frame=59;BTL_A5_enemyVariant_Update(&actors[1]);assert(actors[1].m28_state==0&&!formation.mF1_pad5[1]);
 for(unsigned i=0;i<3;++i){auto& a=actors[i];a.queuedDamage=1000;BTL_A5_enemyVariant_Update(&a);assert(a.m28_state==10&&meters[i].m31==9);for(auto& t:targets[i])assert(t.deleted);BTL_A5_enemyVariant_Update(&a);assert(a.task.isFinished()&&formation.mF4_entityPositions[i]==4);}
 assert(formation.mE0_entityGroup.mF_deadCount==3&&debris==3);formation.m26_counter2=0;formation.m28_state=3;BTL_A5_formationVariant_Update(&formation);assert(formation.m28_state==8&&turn.m8==0&&turn.m0==0&&turn.m2==0&&turn.m4==0&&formation.m1D4_flag==1);for(auto p:pointers)assert(!p);
 for(unsigned i=1;i<77;++i)BTL_A5_formationVariant_Update(&formation);assert(formation.task.isFinished()&&releases==1);
 puts("PASS generated actor/formation encounter handoff: selection, release, launch, turn return, reaction blocking, three deaths, meter rewards and formation retirement; service probes, not hardware");
}
