from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_battleEngine.cpp').read_text()
a=s.index('s32 battleEngine_checkBattleCompletion()');b=s.index('#ifndef SHIPPING_BUILD',a);completion=s[a:b]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text();a=s.index('    case 10:');b=s.index('    default:',a);retirement=s[a:b]
s=(r/'extern/Azel/AzelLib/battle/battleFormationBase.cpp').read_text();a=s.index('void formationCleanupEntityArray(');b=s.index('// 0609b6cc',a);shared=s[a:b]
fixture=r"""
#include <cassert>
#include <cstdint>
#include <vector>
#include <type_traits>
using s32=int32_t;using s16=int16_t;using u8=uint8_t;
constexpr int TASK_FLAGS_FINISHED=1;
struct Task{int m14_flags=0;void markFinished(){m14_flags|=1;}};
struct Actor{Task task;Task* getTask(){return &task;}};using p_workArea=Actor*;
struct sEntityGroup{p_workArea* m0_entityArray;int8_t mE_numEntities,mF_deadCount;};
struct Turn{int m8=1,m0=1,m2=1,m4=1;};
struct Flags{int m80000_hideBattleHUD=1,m10000=0,m8_showingBattleResultScreen=0,m4=0;};
struct s_battleEngine{int m3B2_numBattleFormationRunning=0,m1B8_dragonPitch=8,m1BC_dragonYaw=9,m18C_status=0;u8 m230=0;Flags m188_flags;Turn* m3CC;};
struct Debug{int mFlags[24]{};};struct Overlay{s_battleEngine* m4_battleEngine;Debug* m10_inBattleDebug;};struct Manager{Overlay* m10_battleOverlay;int mE=7;};Manager* gBattleManager;
struct Stats{int m10_currentHP=100;};struct GameState{Stats gameStats;}mainGameState;
struct sBTL_A5_DualFormation{sEntityGroup mE0_entityGroup,m1D0_entityGroup2;int m284_allSwarmDead=0,m288_allSingleDead=0,m28_state=0;s16 m26_counter2=0;Task task;Task* getTask(){return &task;}};
int intro=1,results=0,transitions=0,deaths=0,clears=0;
std::vector<int> released;
void decreaseNPCRefCount(int n){released.push_back(n);}
void battleEngine_FlagQuadrantBitForSafety(int){}void battleEngine_FlagQuadrantBitForDanger(int){}
int battleEngine_isBattleIntroFinished(){return intro;}
void battleEngine_UpdateSub7Sub1(){}void clearVdp2TextMemory(){++clears;}void resetNBG1Map(){}
void battleEngine_resetBattleCameraPreset(){}void battleEngine_restoreCameraDefault(){}
void battleVictoryCamera_create(s_battleEngine*){}
void createBattleResultScreen(s_battleEngine*){++results;}
struct sBattleFormationTransitionTask{};struct sDragonDeathTask{};
int battleFormationTransitionTaskDefinition,dragonDeathTaskDefinition;
template<class T>void createSubTask(s_battleEngine*,int*){if constexpr(std::is_same_v<T,sBattleFormationTransitionTask>)++transitions;else ++deaths;}
// SHARED
// DUAL
void updateFormation(sBTL_A5_DualFormation* pThis){
 ++gBattleManager->m10_battleOverlay->m4_battleEngine->m3B2_numBattleFormationRunning;
 lagiUpdateGarilDualCompletion(pThis);
 switch(pThis->m28_state){
 // RETIREMENT
 default:break;
 }
}
// ENGINE
int main(){
 for(int order=0;order<3;++order)for(int formationId=0;formationId<256;++formationId)for(int special=0;special<2;++special){
  Actor actors[13];p_workArea primary[10],secondary[3];for(int i=0;i<10;++i)primary[i]=&actors[i];for(int i=0;i<3;++i)secondary[i]=&actors[i+10];
  Turn turn;s_battleEngine engine;engine.m3CC=&turn;engine.m230=u8(formationId);engine.m188_flags.m10000=special;
  Debug debug;Overlay overlay{&engine,&debug};Manager manager{&overlay};gBattleManager=&manager;
  sBTL_A5_DualFormation f{{primary,10,0},{secondary,3,0}};
  results=transitions=deaths=clears=0;released.clear();intro=1;
  for(int tick=0;tick<90;++tick){
   if(tick==2){if(order!=1)f.mE0_entityGroup.mF_deadCount=10;if(order!=0)f.m1D0_entityGroup2.mF_deadCount=3;}
   if(tick==5){f.mE0_entityGroup.mF_deadCount=10;f.m1D0_entityGroup2.mF_deadCount=3;}
   if(!(f.task.m14_flags&1))updateFormation(&f);
   const bool alreadyFinished=f.task.m14_flags&1;
   const int completed=battleEngine_checkBattleCompletion();
   if(completed){
    assert(tick==(order==2?79:82));assert(alreadyFinished);assert((released==std::vector<int>{2,12}));
    assert(engine.m18C_status==4&&engine.m1B8_dragonPitch==0&&engine.m1BC_dragonYaw==0&&clears==1&&deaths==0);
    const bool chain=formationId==1||formationId==3||formationId==5||formationId==7||formationId==8;
    assert(results==(!chain||special));assert(transitions==(chain&&!special));
    assert(engine.m188_flags.m8_showingBattleResultScreen==results);
    assert(manager.mE==(chain&&special?0:special?3:0));
    break;
   }
   assert(results==0&&transitions==0&&deaths==0);
   assert(tick<89);
  }
 }
 // Empty formation count during an unfinished intro must not open results.
 Turn turn;s_battleEngine engine;engine.m3CC=&turn;Debug debug;Overlay overlay{&engine,&debug};Manager manager{&overlay};gBattleManager=&manager;
 intro=0;results=transitions=0;assert(!battleEngine_checkBattleCompletion());assert(!results&&!transitions);
}
""".replace('// SHARED',shared).replace('// DUAL',(r/'src/integration/lagi_garil_dual_completion.inc').read_text()).replace('// RETIREMENT',retirement).replace('// ENGINE',completion)
with tempfile.TemporaryDirectory(prefix='lagi-dual-result-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
print('PASS 1536 dual lifecycle/shared completion scenarios: both death orders, simultaneous defeat, native delay, ordered release, result/chain selection and intro gate')
