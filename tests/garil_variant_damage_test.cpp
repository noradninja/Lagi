#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "lagi/native_battle_models.h"
using u8=uint8_t;using u32=uint32_t;
struct s_workAreaCopy{struct Task{void markFinished(){}}task;Task* getTask(){return &task;}};
struct sVec3_FP{int x=0,y=0,z=0;};
struct sBattleTargetable{u32 m50_flags=0;int16_t m58=0;int8_t m60=0,m5E_impactForce=0;sVec3_FP* mC=nullptr;};
struct Definition{unsigned m4_count=2;};
struct s_3dModel{unsigned m50_targetBoneCount=1,m52_targetableCount=2;std::vector<Definition>* m40=nullptr;std::vector<std::vector<sVec3_FP>> m44_hotpointData;sVec3_FP* m58_targetablePositions=nullptr;sBattleTargetable* m54_targetables=nullptr;};
struct Formation{std::array<u8,3> mF4_entityPositions{};}formation;
struct Meter{u8 m31=0;}meter;
struct sBTL_A5_VariantEnemy:s_workAreaCopy{u32 m14_flags=0;int16_t mB2_health=100,m_lagiPendingDamage=0,m14C_lightColor=0;Meter* mAC_lifeMeter=&meter;sVec3_FP m30_position;std::vector<s_3dModel> m2C_models=std::vector<s_3dModel>(1);LagiBattleModelTargetState m_lagiTargets;sBTL_A5_VariantEnemy* unused=nullptr;Formation* m158_formation=&formation;int m1C_entityIndex=1;};
struct Engine{u8 m22C_dragonCurrentQuadrant=0;struct{bool m1000=false;}m188_flags;sVec3_FP m1A0_battleAutoScrollDelta;}engine;
struct Overlay{Engine* m4_battleEngine=&engine;}overlay;struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
unsigned explosions=0,sparks=0,numbers=0;int shownDamage=0;
void createExplosionAtPosition(sVec3_FP* point,sVec3_FP* scroll,int scale,u8 variant){assert(point&&scroll==&engine.m1A0_battleAutoScrollDelta&&scale==0x20000&&variant==1);++explosions;}
void createHitSparkEffect(s_workAreaCopy*,sBattleTargetable&,int mode){assert(!mode);++sparks;}
void createDamageDisplayTask(s_workAreaCopy*,int16_t damage,sVec3_FP*,int mode){assert(mode==1);++numbers;shownDamage=damage;}
#include "shared.inc"
#include "variant.inc"
int main(){
 for(unsigned actorQuadrant=0;actorQuadrant<4;++actorQuadrant)for(unsigned dragonQuadrant=0;dragonQuadrant<4;++dragonQuadrant)for(bool window:{false,true}){
  sBTL_A5_VariantEnemy actor;formation.mF4_entityPositions[1]=actorQuadrant;engine.m22C_dragonCurrentQuadrant=dragonQuadrant;engine.m188_flags.m1000=window;meter.m31=0x80;explosions=sparks=numbers=0;
  std::vector<Definition> definitions(1);std::array<sBattleTargetable,2> targets;std::array<sVec3_FP,2> positions;auto& model=actor.m2C_models[0];model.m40=&definitions;model.m54_targetables=targets.data();model.m58_targetablePositions=positions.data();model.m44_hotpointData={{{11,22,33},{44,55,66}}};
  actor.m_lagiTargets.damageCallbackEA={0x60A3A4C,0x60A3A50,0x60A3A74,0x60A3A98};
  for(unsigned i=0;i<2;++i){targets[i].m50_flags=0xA0000;targets[i].m58=i?9:7;targets[i].m5E_impactForce=i;targets[i].mC=&positions[i];}
  lagiUpdateGarilVariantDamageValues(&actor);assert(lagiUpdateGarilVariantDamage(&actor));
  const auto value=dragonQuadrant==((actorQuadrant+2)&3)?45:5;
  for(auto& target:targets)assert(!target.m50_flags&&target.m60==value);
  assert(actor.mB2_health==84&&actor.m_lagiPendingDamage==(window?0:16)&&meter.m31==0x82&&actor.m14C_lightColor==value);
  assert(sparks==2&&explosions==1&&numbers==unsigned(window));if(window)assert(shownDamage==16);
  assert(positions[0].x==11&&positions[1].z==66);
  actor.m14_flags|=0x10000000;targets[0].m50_flags=0x80000;assert(!lagiUpdateGarilVariantDamage(&actor));assert(targets[0].m50_flags==0x80000&&actor.mB2_health==84);
 }
 sBTL_A5_VariantEnemy actor;actor.m_lagiTargets.hitboxValues={1,2,3,4};formation.mF4_entityPositions[1]=4;lagiUpdateGarilVariantDamageValues(&actor);assert((actor.m_lagiTargets.hitboxValues==std::array<int16_t,4>{1,2,3,4}));
 puts("PASS production variant adapter plus shared damage: quadrant values, live targets, callbacks, accumulation/display, health, meter and retired gate");
}
