#define LAGI_TUTORIAL_TASK_FIXTURE_ONLY
#include "tutorial_laser_enemy_task_test.cpp"
struct EngineTimer {int16_t m0=0,m2=0,m4=0;int32_t m8=0;} engineTimer;
struct Engine {int8_t m3B2_numBattleFormationRunning=0;struct {bool m80=false;}m188_flags;
 EngineTimer* m3CC=&engineTimer;sVec3_FP mC_battleCenter{};int32_t m474_XPReceivedFromBattle=0,m478_dyneReceivedFromBattle=0;}engine;
struct Overlay {Engine* m4_battleEngine=&engine;}overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;}manager;
static Manager* gBattleManager=&manager;
static npcFileDeleter resources;
static unsigned allocationsNPC=0,releasesNPC=0,timerSetups=0,names=0;
static npcFileDeleter* allocateNPC(s_workAreaCopy* owner,int slot){assert(slot==20&&owner==created.back());++allocationsNPC;return &resources;}
static void decreaseNPCRefCount(int slot){assert(slot==20);++releasesNPC;}
static void battleEngine_UpdateSub1Sub0(int value){assert(value==0&&engineTimer.m2==120);++timerSetups;}
static void displayFormationName(int name,int mode,int arg){assert(name==6&&mode==0&&arg==9);++names;}
static sVec3_FP readSaturnVec3(sSaturnPtr p){return {bits(readSaturnEA(p).ea),bits(readSaturnEA(p+4).ea),bits(readSaturnEA(p+8).ea)};}
#include "../src/integration/lagi_tutorial_laser_formation.inc"
static void putWord(unsigned offset,uint32_t v){for(unsigned b=0;b<4;++b)disc[offset+b]=v>>(24-8*b);}
int main(){
 allowNestedParents=true;repeatedModelDescriptor=true;
 s_workAreaCopy parent;expectedParent=&parent;s_fileBundle bundle;sAnimationData animation{8};bundle.animation=&animation;
 parent.m0_fileBundle=&bundle;resources.m0_fileBundle=&bundle;
 Probe probe;LagiTutorialLaserEnemyHooks hooks{{&probe,quadrant,displayWindow,onHit,hitDisplay,damageDisplay},random,sound,death,hide};
 sSaturnPtr positions{16},modelEA{0};descriptor(0,0x10,0);
 putWord(16,0x12000);putWord(20,0);putWord(24,0);putWord(28,0xfffee000);putWord(32,0);putWord(36,0);
 const std::array<uint32_t,4> callbacks{0x0609f440,0x0609f444,0x0609f468,0x0609f48c};
 LagiTutorialLaserFormationSpawn spawn{&parent,positions,modelEA,callbacks,hooks};
 auto* form=static_cast<sLagiTutorialLaserFormation*>(lagiCreateTutorialLaserFormation(spawn));
 assert(form&&created.size()==3&&allocationsNPC==1&&timerSetups==1&&names==1&&engineTimer.m2==120&&probe.rng==2);
 auto* first=static_cast<sLagiTutorialLaserEnemyTask*>(form->enemies[0]);auto* second=static_cast<sLagiTutorialLaserEnemyTask*>(form->enemies[1]);
 assert(first&&second&&form->deadCount==0&&first->authoredOffset.x==0x12000&&second->authoredOffset.x==-0x12000);
 assert(first->formationPosition==&form->position&&second->formationRotation==&form->rotation&&first->deadCount==&form->deadCount);
 assert(first->experience==&engine.m474_XPReceivedFromBattle&&second->dyne==&engine.m478_dyneReceivedFromBattle);
 assert(form->m_DeleteMethod&&!form->m_DrawMethod);
 engine.mC_battleCenter={0x100000,0x200000,0x300000};engine.m188_flags.m80=true;engineTimer.m8=1;
 form->m_UpdateMethod(form);assert(engine.m3B2_numBattleFormationRunning==1&&form->position.x==0x100000&&form->counter==0&&!form->allDead&&engineTimer.m8==1);
 engine.m188_flags.m80=false;engine.m3B2_numBattleFormationRunning=127;
 form->m_UpdateMethod(form);assert(engine.m3B2_numBattleFormationRunning==-128&&engineTimer.m8==1&&!form->getTask()->finished);
 form->deadCount=2;
 for(int frame=0;frame<62;++frame){
  engineTimer={1,2,3,4};engine.m3B2_numBattleFormationRunning=0;form->m_UpdateMethod(form);
  assert(form->allDead&&engine.m3B2_numBattleFormationRunning==1&&engineTimer.m0==0&&engineTimer.m2==0&&engineTimer.m4==0&&engineTimer.m8==0);
  assert(form->getTask()->finished==(frame==61));assert(form->counter==(frame==61?0:frame+1));
 }
 form->m_DeleteMethod(form);assert(releasesNPC==1);
 // Every signed native timer boundary; only the native >60 gate finishes.
 for(int initial=-32768;initial<=32767;++initial){
  form->counter=int16_t(initial);form->getTask()->finished=false;form->m_UpdateMethod(form);
  assert(form->getTask()->finished==(initial>60));
  assert(form->counter==(initial>60?0:lagiBattleDamageWord(uint32_t(initial)+1)));
 }
 // Child creation failure increments authored group count, rather than adding
 // a fake running enemy. Exactly the failed child is marked finished.
 auto count=created.size();probe={};creates=steps=0;failModel=1;
 auto* partial=static_cast<sLagiTutorialLaserFormation*>(lagiCreateTutorialLaserFormation(spawn));
 assert(partial&&created.size()==count+3&&partial->enemies[0]&&!partial->enemies[1]&&partial->deadCount==1);
 assert(created.back()->getTask()->finished&&meters.back()->getTask()->finished);
 auto invalid=spawn;invalid.services.random=nullptr;count=created.size();assert(!lagiCreateTutorialLaserFormation(invalid)&&created.size()==count);
 delete form;delete partial;
 // Formation tasks were deleted above; other entries are concrete actor tasks.
 for(auto* t:created)if(t!=form&&t!=partial)delete static_cast<sLagiTutorialLaserEnemyTask*>(t);
 for(auto* t:meters)delete t;release();
 std::puts("PASS: actual tutorial formation/task/constructor/update wiring, authored pair, resource ownership, timer gating, 65536 completion boundaries and child-failure accounting; mocked kernel/engine services, not hardware");
}
