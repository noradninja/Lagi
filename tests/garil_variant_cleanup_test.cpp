#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u8=uint8_t;using u32=uint32_t;
struct s_workAreaCopy{struct Task{bool finished=false;void markFinished(){finished=true;}}task;Task* getTask(){return &task;}};
struct sBattleTargetable{bool retired=false;};
struct s_3dModel{unsigned m52_targetableCount=0;sBattleTargetable* m54_targetables=nullptr;};
struct Formation{std::array<u8,3> mF1_pad5{};}formation;
struct Meter:s_workAreaCopy{u8 m31=0;}meter;
struct sBTL_A5_VariantEnemy:s_workAreaCopy{u32 m14_flags=0;std::vector<s_3dModel> m2C_models;Formation* m158_formation=&formation;int m1C_entityIndex=1;Meter* mAC_lifeMeter=&meter;};
unsigned retired=0;
void deleteTargetable(sBattleTargetable* t){assert(!t->retired);t->retired=true;++retired;}
#include "lagi_battle_enemy_lifecycle.h"
// RETIRE
// REACTION
// CLEANUP
int main(){
 for(bool defeated:{false,true}){
  sBTL_A5_VariantEnemy actor;std::array<sBattleTargetable,3> targets;actor.m2C_models={{2,targets.data()},{1,targets.data()+2}};formation.mF1_pad5[1]=7;meter.task.finished=false;meter.m31=0x40;retired=0;
  if(defeated){lagiRetireBattleEnemyModelTargets(&actor,actor.m14_flags,actor.m2C_models,nullptr);lagiBattleEnemyDefeated(actor.mAC_lifeMeter);}
  lagiDeleteGarilVariantActor(&actor);assert(retired==3);assert((actor.m14_flags&0x90000000u)==0x90000000u);assert(formation.mF1_pad5[1]==5);
  if(defeated){assert(actor.mAC_lifeMeter==&meter&&!meter.task.finished&&meter.m31==0x49);}
  else{assert(!actor.mAC_lifeMeter&&meter.task.finished&&meter.m31==0x40);}
  lagiDeleteGarilVariantActor(&actor);assert(retired==3);
 }
 sBTL_A5_VariantEnemy failed;failed.mAC_lifeMeter=nullptr;lagiDeleteGarilVariantActor(&failed);
 puts("PASS production variant cleanup with shared retirement: live cancellation, completed death rewards, repeated cleanup and empty initialization");
}
