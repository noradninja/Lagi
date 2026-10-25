#define LAGI_TUTORIAL_ENEMY_FIXTURE_ONLY
#include "tutorial_laser_enemy_test.cpp"
template<class T> struct s_workAreaTemplateWithCopy:s_workAreaCopy {
 using Fn=void(*)(T*);
 struct TypedTaskDefinition {Fn m_pInit,m_pUpdate,m_pDraw,m_pDelete;};
 Fn m_UpdateMethod=nullptr,m_DrawMethod=nullptr,m_DeleteMethod=nullptr;
};
static std::vector<s_workAreaCopy*> created;
static s_workAreaCopy* expectedParent=nullptr;
static bool allowNestedParents=false;
template<class T>static T* createSubTaskWithCopy(s_workAreaCopy* parent,const typename T::TypedTaskDefinition* def){
 assert(parent&&(allowNestedParents||parent==expectedParent)&&!def->m_pInit&&def->m_pUpdate);
 if(!allowNestedParents)assert(!def->m_pDelete&&def->m_pDraw);
 auto* t=new T{};t->m0_fileBundle=parent->m0_fileBundle;t->m_UpdateMethod=def->m_pUpdate;t->m_DrawMethod=def->m_pDraw;t->m_DeleteMethod=def->m_pDelete;created.push_back(t);return t;
}
struct sEnemyLifeMeterTask:s_workAreaCopy {uint8_t m31=0;sVec3_FP* position=nullptr;int16_t* health=nullptr;};
static std::vector<sEnemyLifeMeterTask*> meters;
static sEnemyLifeMeterTask* createEnemyLifeMeterTask(sVec3_FP* pos,int maximum,int16_t* hp,int16_t index){
 assert(maximum==300&&index==-1&&*hp==300);auto* t=new sEnemyLifeMeterTask{};t->position=pos;t->health=hp;meters.push_back(t);return t;
}
static int initAnimation(s_3dModel* m,sAnimationData* animation){m->m30_pCurrentAnimation=animation;return 1;}
static uint32_t performModulo2(uint32_t d,uint32_t n){return d?n%d:0;}
static void setAnimationFrame(s_3dModel*,int32_t){}
#include "../src/integration/lagi_tutorial_laser_enemy_init.inc"
static unsigned draws=0;
static const sVec3_FP* drawnPosition=nullptr;
void lagiDrawBattleEnemyModels(uint32_t&,int16_t,const sVec3_FP& position,const sVec3_FP&,std::vector<s_3dModel>& models){assert(models.size()==1);++draws;drawnPosition=&position;}
#include "../src/integration/lagi_tutorial_laser_enemy_task.inc"
#ifndef LAGI_TUTORIAL_TASK_FIXTURE_ONLY
int main(){
 s_workAreaCopy parent;expectedParent=&parent;parent.header.fill(0xa5);
 s_fileBundle bundle;sAnimationData animation{8};bundle.animation=&animation;parent.m0_fileBundle=&bundle;
 npcFileDeleter resources;resources.m0_fileBundle=&bundle;
 sVec3_FP position{100,200,300},rotation{},offset{0x12000,0,0};
 int8_t deadCount=0;int32_t xp=0,dyne=0;Probe probe;
 const LagiTutorialLaserEnemyHooks services{{&probe,quadrant,displayWindow,onHit,hitDisplay,damageDisplay},random,sound,death,hide};
 sSaturnPtr descriptorEA{0};const std::array<uint32_t,4> callbacks{0x0609f440,0x0609f444,0x0609f468,0x0609f48c};
 LagiTutorialLaserEnemySpawn spawn{&parent,&resources,position,rotation,deadCount,xp,dyne,offset,0,descriptorEA,callbacks,services};
 auto create=[&](int index){creates=steps=0;failModel=-1;probe={};spawn.entityIndex=index;descriptor(0,0x10,0);return static_cast<sLagiTutorialLaserEnemyTask*>(lagiCreateTutorialLaserEnemyTask(spawn));};
 auto* first=create(0);auto* second=create(1);
 assert(first&&second&&first!=second&&created.size()==2&&meters.size()==2);
 assert(first->entityIndex==0&&second->entityIndex==1&&first->health==300&&second->health==300);
 assert(first->lifeMeter!=second->lifeMeter&&first->lifeMeter->health==&first->health&&second->lifeMeter->health==&second->health);
 assert(first->formationPosition==&position&&second->formationRotation==&rotation&&first->deadCount==&deadCount);
 assert(first->models.data()!=second->models.data()&&first->m_DeleteMethod==nullptr&&second->m_DeleteMethod==nullptr);
 // Actual task callbacks assemble the correct actor views and preserve parent
 // references across movement. Service context is shared, actor state is not.
 position={1000,2000,3000};probe={};probe.animationBefore=steps;
 first->m_UpdateMethod(first);assert(first->position.x==1000+0x12000&&first->position.y==2000&&second->position.x==0);
 first->m_DrawMethod(first);assert(draws==1&&drawnPosition==&first->position);
 // Retirement/debris disables ONLY this actor's real typed draw callback.
 first->state=2;auto firstView=first->view();liveActor=&firstView;probe={};probe.animationBefore=steps;
 first->m_UpdateMethod(first);assert(!first->m_DrawMethod&&second->m_DrawMethod&&probe.hides==0&&debrisCalls==1&&first->state==3);
 second->m_DrawMethod(second);assert(draws==2&&drawnPosition==&second->position);
 // Final task state writes the shared group count and finishes the actual task.
 first->deathTimer=31;probe={};probe.animationBefore=steps;first->m_UpdateMethod(first);
 assert(first->getTask()->finished&&deadCount==1&&!second->getTask()->finished);
 // Incomplete bindings do not create an immortal or pretend task.
 auto size=created.size();auto broken=spawn;broken.services.damage.hitCallback=nullptr;
 assert(!lagiCreateTutorialLaserEnemyTask(broken)&&created.size()==size);
 auto missingResources=spawn;missingResources.resources=nullptr;assert(!lagiCreateTutorialLaserEnemyTask(missingResources)&&created.size()==size);
 // Real constructor failure leaves a finished task and finishes its separate
 // life-meter owner, with no child update exposed as a live formation entry.
 creates=steps=0;failModel=0;probe={};
 assert(!lagiCreateTutorialLaserEnemyTask(spawn));auto* failed=static_cast<sLagiTutorialLaserEnemyTask*>(created.back());
 assert(failed->getTask()->finished&&failed->lifeMeter->getTask()->finished);
 for(auto byte:parent.header)assert(byte==0xa5);
 for(auto* t:created)delete static_cast<sLagiTutorialLaserEnemyTask*>(t);
 for(auto* t:meters)delete t;
 release();
 std::puts("PASS: actual typed tutorial task factory, independent actors, shared formation references, native callback wiring and failed setup; task/model/presentation services mocked, not hardware");
}
#endif
