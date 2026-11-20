#define main modelLifecycleFixtureMain
#include "native_battle_models_test.cpp"
#undef main
#include "lagi/native_formation_motion.h"
#include "lagi/native_tutorial_laser_enemy.h"
struct sEnemyLifeMeterTask {Task task;sVec3_FP* position;int16_t* health;int max=0;int index=0;Task* getTask(){return &task;}} meter;
static unsigned meterCalls=0,animCalls=0,randomCalls=0,moduloCalls=0,frameCalls=0;
static uint32_t rngValue=0,divisor=0,frameRemainder=0;
static int32_t selectedFrame=0;
static bool failMeter=false;
static sEnemyLifeMeterTask* createEnemyLifeMeterTask(sVec3_FP* position,int maximum,int16_t* health,int16_t index){
 assert(*health==300&&maximum==300&&index==-1&&creates==0&&steps==0);++meterCalls;
 meter={};meter.position=position;meter.health=health;meter.max=maximum;meter.index=index;return failMeter?nullptr:&meter;
}
static int initAnimation(s_3dModel* model,sAnimationData* animation){assert(meterCalls==1&&creates==1&&steps==1&&randomCalls==0);++animCalls;model->m30_pCurrentAnimation=animation;return 1;}
static uint32_t random(void*){assert(animCalls==1&&steps==1&&frameCalls==0);++randomCalls;return rngValue;}
static uint32_t performModulo2(uint32_t d,uint32_t n){assert(randomCalls==1&&n==rngValue);++moduloCalls;divisor=d;frameRemainder=d?n%d:0;return frameRemainder;}
static void setAnimationFrame(s_3dModel*,int32_t frame){assert(moduloCalls==1&&steps==1);++frameCalls;selectedFrame=frame;}
#include "../src/integration/lagi_tutorial_laser_enemy_init.inc"
int main(){
 s_workAreaCopy owner;owner.header.fill(0xa5);s_fileBundle bundle;sAnimationData animation;
 sEnemyLifeMeterTask* life=nullptr;int16_t hp=0;int32_t entity=0,rate=0;uint8_t count=0;
 sVec3_FP pos{},cur{},target{},vel{},acc{},rot{},rotCur{},rotTarget{},rotVel{},rotAcc{},parent{},parentRot{};
 LagiBattleModelMotionView motion{pos,cur,target,vel,acc,rot,rotCur,rotTarget,rotVel,rotAcc,parent,parentRot,0};
 LagiBattleModelTargetState state;std::vector<s_3dModel> models;
 LagiTutorialLaserEnemyInitView actor{&owner,&bundle,models,count,state,life,hp,entity,motion,rate};
 sVec3_FP offset{0x12000,0,-0x12000};
 const std::array<uint32_t,4> callbacks={0x0609f440,0x0609f444,0x0609f468,0x0609f48c};
 auto reset=[&](){owner.task.finished=false;life=nullptr;hp=-1;entity=-1;rate=-1;count=0;models.clear();creates=steps=0;failModel=-1;failMeter=false;
  meterCalls=animCalls=randomCalls=moduloCalls=frameCalls=0;meter={};hotspotCalls.clear();state.healthFloor=-99;state.hitboxValues.fill(-99);
  pos={9,8,7};cur={6,5,4};target={3,2,1};vel={111,222,333};acc={-1,-2,-3};rot={5,6,7};rotCur=rotTarget={1,2,3};rotVel={11,22,33};rotAcc={44,55,66};
  descriptor(0,0x10,0x060aad40);};
 const uint32_t rngs[]={0,1,2,0x7fffffff,0x80000000,0xfffffffe,0xffffffff};
 size_t cases=0;
 for(unsigned frames=0;frames<=65535;++frames)for(auto rng:rngs){
  reset();animation.m4_numFrames=uint16_t(frames);bundle.animation=&animation;rngValue=rng;
  assert(lagiInitTutorialLaserEnemy(actor,1,offset,{0},callbacks,random,nullptr));
  assert(owner.m0_fileBundle==&bundle&&entity==1&&hp==300&&life==&meter&&!meter.task.finished);
  assert(meter.position==&pos&&meter.health==&hp&&meter.max==300&&meter.index==-1);
  assert(count==1&&models.size()==1&&creates==1&&steps==2&&randomCalls==1&&frameCalls==1);
  const uint32_t expectedDivisor=uint32_t(frames)-1;
  assert(divisor==expectedDivisor&&uint32_t(selectedFrame)==(expectedDivisor?rng%expectedDivisor:0));
  assert(state.healthFloor==0&&state.damageCallbackEA==callbacks&&(state.hitboxValues==std::array<int16_t,4>{30,1,10,1}));
  assert(target.x==offset.x&&target.y==offset.y&&target.z==offset.z&&cur.x==offset.x&&cur.z==offset.z&&rate==0xf5c);
  assert(pos.x==9&&pos.y==8&&pos.z==7&&rot.x==0&&rot.y==0x08000000&&rot.z==0&&rotCur.y==rot.y&&rotTarget.y==rot.y);
  assert(vel.x==111&&acc.x==-1&&rotVel.x==11&&rotAcc.x==44);++cases;
 }
 reset();bundle.animation=nullptr;rngValue=0xffffffff;
 assert(lagiInitTutorialLaserEnemy(actor,0,offset,{0},callbacks,random,nullptr));assert(divisor==0xffffffff&&selectedFrame==0);
 reset();failModel=0;
 assert(!lagiInitTutorialLaserEnemy(actor,0,offset,{0},callbacks,random,nullptr));
 assert(owner.task.finished&&life==&meter&&meter.task.finished&&randomCalls==0&&animCalls==0&&steps==0&&state.healthFloor==-99);
 reset();failMeter=true;
 assert(!lagiInitTutorialLaserEnemy(actor,0,offset,{0},callbacks,random,nullptr));assert(owner.task.finished&&!life&&creates==0&&randomCalls==0);
 reset();LagiTutorialLaserEnemyInitView missing{&owner,nullptr,models,count,state,life,hp,entity,motion,rate};
 assert(!lagiInitTutorialLaserEnemy(missing,0,offset,{0},callbacks,random,nullptr));assert(owner.task.finished&&meterCalls==0&&creates==0);
 for(auto byte:owner.header)assert(byte==0xa5);
 std::printf("PASS: actual tutorial constructor, %zu frame/RNG cases, null animation, named-field preservation and failed life-meter/model cleanup; services mocked, not hardware\n",cases);
}
