// Reuse the typed task/model/allocator fixture, including its creation and
// retirement checks. Engine services stay mocked; production damage code below.
#define main modelLifecycleFixtureMain
#include "native_battle_models_test.cpp"
#undef main
#include "../src/integration/lagi_battle_model_damage.inc"
#include <limits>
struct Probe {
 unsigned quadrant=0,queries=0,windows=0,hits=0,displays=0,numbers=0;
 bool window=false,killOnCallback=false,turnOnCallback=false;
 uint32_t callbackEA=0;
 int16_t before=0,displayHealth=0,number=0;
 LagiBattleModelDamageView* actor=nullptr;
};
static uint8_t quadrant(void* p){auto& s=*static_cast<Probe*>(p);++s.queries;return uint8_t(s.quadrant);}
static bool window(void* p){auto& s=*static_cast<Probe*>(p);++s.windows;return s.window;}
static void hit(void* p,uint32_t ea,s_workAreaCopy* owner,sBattleTargetable& t){
 auto& s=*static_cast<Probe*>(p);++s.hits;s.before=s.actor->health;s.callbackEA=ea;
 assert(owner==s.actor->owner&&!(t.m50_flags&0xa0000u));
 assert(s.actor->flags&0x800000u);assert(s.actor->lightColor==uint8_t(t.m60));
 if(s.killOnCallback)s.actor->flags|=0x80000000u;
 if(s.turnOnCallback)s.quadrant=(s.quadrant+1)&3;
}
static void display(void* p,s_workAreaCopy* owner,sBattleTargetable&,int mode){
 auto& s=*static_cast<Probe*>(p);assert(owner==s.actor->owner&&mode==0);++s.displays;s.displayHealth=s.actor->health;
}
static void number(void* p,s_workAreaCopy* owner,int16_t value,sVec3_FP* pos,int mode){
 auto& s=*static_cast<Probe*>(p);assert(owner==s.actor->owner&&pos==&s.actor->worldPosition&&mode==1);++s.numbers;s.number=value;
}
static int16_t word(int32_t v){auto low=uint32_t(v)&65535;return int16_t(low>=32768?int32_t(low)-65536:low);}
int main(){
 assert(modelLifecycleFixtureMain()==0);
 s_workAreaCopy owner;owner.header.fill(0xa5);
 uint32_t flags=0;
 int16_t health=0,floor=0,pending=0,color=0;uint8_t meter=0;
 sVec3_FP world{100,200,300},position{};sBattleTargetable target{};
 std::vector<s_hotpointDefinition> definitions(1);definitions[0].m4_count=1;definitions[0].m0.resize(1);
 std::vector<s_3dModel> models(1);
 auto& m=models[0];m.m40=&definitions;m.m12_numBones=m.m50_targetBoneCount=m.m52_targetableCount=1;
 m.m44_hotpointData={{{11,22,33}}};m.m58_targetablePositions=&position;m.m54_targetables=&target;
 LagiBattleModelTargetState state;state.damageCallbackEA={0x100,0x200,0x300,0x400};state.hitboxValues={-1,10,0x1234,-32768};
 LagiBattleModelDamageView actor{&owner,flags,health,floor,pending,color,meter,world,models,state};
 Probe probe;probe.actor=&actor;
 LagiBattleModelDamageHooks hooks{&probe,quadrant,window,hit,display,number};
 const int16_t edges[]={-32768,-32767,-100,-1,0,1,2,10,99,32766,32767};
 const int16_t floors[]={-32768,-1,0,1,32767};
 size_t cases=0;
 for(auto hp:edges)for(auto damage:edges)for(auto minimum:floors)for(int kind=-1;kind<5;++kind)for(int q=0;q<4;++q)for(int canShow=0;canShow<2;++canShow){
  flags=0x44;health=hp;floor=minimum;pending=hp;color=-999;meter=0x80;
  target.m50_flags=0xa0000u|0x101;target.m58=damage;target.m5E_impactForce=int8_t(kind);
  position={};probe={};probe.actor=&actor;probe.quadrant=q;probe.window=canShow;
  const auto postSub=word(int32_t(hp)-damage);
  const auto expected=(minimum!=0&&postSub<=minimum)?minimum:postSub;
  const auto accumulated=word(int32_t(hp)+damage);
  const bool flush=expected<=0||(canShow&&accumulated>0);
  assert(lagiUpdateBattleEnemyModelDamage(actor,hooks));
  assert(health==expected&&pending==(flush?0:accumulated));assert(uint8_t(meter)==0x82);
  assert(position.x==11&&position.y==22&&position.z==33);
  assert(target.m50_flags==0x101&&color==uint8_t(state.hitboxValues[q]));
  assert(probe.hits==unsigned(kind>=0&&kind<4));
  if(probe.hits){assert(probe.before==hp&&probe.callbackEA==state.damageCallbackEA[kind]);}
  assert(probe.displays==unsigned(postSub>=0));if(probe.displays)assert(probe.displayHealth==postSub);
  assert(probe.numbers==unsigned(flush));if(flush)assert(probe.number==accumulated);
  assert(probe.windows==unsigned(expected>0));++cases;
 }
 // Overrides: later matching rows win, signed model IDs do not alias indices,
 // and native word values are deliberately truncated to one byte.
 LagiBattleTargetOverride overrides[]={{-1,0,{1,2,3,4}},{0,0,{0x101,0x202,0x303,0x404}},{0,0,{-1,-2,-3,-4}}};
 state.auxiliaryCount=3;state.auxiliaryData=overrides;
 for(unsigned q=0;q<4;++q){
  probe={};probe.actor=&actor;probe.quadrant=q;health=100;pending=0;flags=0;target.m50_flags=0;
  assert(!lagiUpdateBattleEnemyModelDamage(actor,hooks));assert(uint8_t(target.m60)==uint8_t(-1-int(q)));
 }
 // Retired gate returns before copying positions, querying quadrant or flushing.
 probe={};probe.actor=&actor;flags=0x10000000;health=-1;pending=99;position={};target.m50_flags=0xa0000;
 assert(!lagiUpdateBattleEnemyModelDamage(actor,hooks));assert(position.x==0&&probe.queries==0&&probe.numbers==0&&pending==99&&target.m50_flags==0xa0000);
 // Dying but not retired refreshes target positions/values without consuming hits.
 probe={};probe.actor=&actor;flags=0x80000000;health=100;pending=0;position={};
 assert(!lagiUpdateBattleEnemyModelDamage(actor,hooks));assert(position.x==11&&probe.queries>0&&probe.hits==0&&target.m50_flags==0xa0000);
 // Invalid authored target reference fails safely, not an out-of-bounds write.
 overrides[1].targetIndex=-1;flags=0;owner.task.finished=false;
 assert(!lagiUpdateBattleEnemyModelDamage(actor,hooks));assert(owner.task.finished);overrides[1].targetIndex=0;
 state.auxiliaryCount=0;state.auxiliaryData=nullptr;
 // Callback sets dead flag inside a model: remaining hits in this model still
 // run, but the following model only updates its target values/positions.
 std::vector<s_hotpointDefinition> two(1);two[0].m4_count=2;two[0].m0.resize(2);
 sBattleTargetable targets[3]{};sVec3_FP positions[3]{};models.resize(2);
 models[0].m40=&two;models[0].m12_numBones=models[0].m50_targetBoneCount=1;models[0].m52_targetableCount=2;
 models[0].m44_hotpointData={{{1,2,3},{4,5,6}}};models[0].m54_targetables=targets;models[0].m58_targetablePositions=positions;
 models[1].m40=&definitions;models[1].m12_numBones=models[1].m50_targetBoneCount=models[1].m52_targetableCount=1;
 models[1].m44_hotpointData={{{7,8,9}}};models[1].m54_targetables=targets+2;models[1].m58_targetablePositions=positions+2;
 for(auto& t:targets){t.m50_flags=0xa0000;t.m58=1;t.m5E_impactForce=0;}
 probe={};probe.actor=&actor;probe.killOnCallback=true;probe.turnOnCallback=true;flags=0;health=100;floor=0;pending=0;
 assert(lagiUpdateBattleEnemyModelDamage(actor,hooks));assert(probe.hits==2&&health==98&&pending==2);
 assert(targets[0].m50_flags==0&&targets[1].m50_flags==0&&targets[2].m50_flags==0xa0000);
 assert(uint8_t(targets[2].m60)==uint8_t(state.hitboxValues[2])&&positions[2].x==7);
 for(auto b:owner.header)assert(b==0xa5);
 std::printf("PASS: %zu shared native damage boundary/order cases plus overrides, live callbacks, death/retirement gates; mocked services, not hardware\n",cases);
}
