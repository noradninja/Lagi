from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'extern/Azel/AzelLib/battle/battleFormationBase.cpp').read_text()
a=s.index('void formationCleanupEntityArray(');b=s.index('// 0609b6cc',a)
shared=s[a:b]
fixture=r"""
#include <cassert>
#include <cstdint>
#include <vector>
using s32=int32_t;
constexpr int TASK_FLAGS_FINISHED=1;
struct Task{int m14_flags=0;};struct Actor{Task task;Task* getTask(){return &task;}};using p_workArea=Actor*;
struct sEntityGroup{p_workArea* m0_entityArray;int8_t mE_numEntities,mF_deadCount;};
struct Turn{int m8=9,m0=8,m2=7,m4=6;};struct Engine{Turn* m3CC;};struct Overlay{Engine* m4_battleEngine;};struct Manager{Overlay* m10_battleOverlay;};Manager* gBattleManager;
struct sBTL_A5_DualFormation{sEntityGroup mE0_entityGroup,m1D0_entityGroup2;int m284_allSwarmDead=0,m288_allSingleDead=0,m28_state=3;};
std::vector<int> events;
void battleEngine_FlagQuadrantBitForSafety(int n){events.push_back(n);}
void battleEngine_FlagQuadrantBitForDanger(int n){events.push_back(100+n);}
// SHARED
// COMPLETION
int main(){
 for(int mask=0;mask<8192;++mask)for(int deaths=0;deaths<4;++deaths){
  Actor actors[13];p_workArea primary[10],secondary[3];
  for(int i=0;i<13;++i){actors[i].task.m14_flags=(mask>>i)&1;if(i<10)primary[i]=&actors[i];else secondary[i-10]=&actors[i];}
  Turn turn;Engine engine{&turn};Overlay overlay{&engine};Manager manager{&overlay};gBattleManager=&manager;
  sBTL_A5_DualFormation f{{primary,10,int8_t(deaths&1?10:9)},{secondary,3,int8_t(deaths&2?3:2)}};
  events.clear();lagiUpdateGarilDualCompletion(&f);
  assert(f.m284_allSwarmDead==bool(deaths&1)&&f.m288_allSingleDead==bool(deaths&2));
  assert(f.m28_state==(deaths==3?10:3));
  for(int i=0;i<13;++i)assert((i<10?primary[i]:secondary[i-10])==((mask>>i)&1?nullptr:&actors[i]));
  std::vector<int> expected;if(deaths&1)expected={0,1,114};if(deaths&2)expected.insert(expected.end(),{0,4,101});assert(events==expected);
  assert(turn.m8==(deaths==3?0:9)&&turn.m0==(deaths==3?0:8)&&turn.m2==(deaths==3?0:7)&&turn.m4==(deaths==3?0:6));
 }
}
""".replace('// SHARED',shared).replace('// COMPLETION',(r/'src/integration/lagi_garil_dual_completion.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-dual-completion-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
assert data[0x60A2C50-base:0x60A2C64-base].hex()=='844f6303844e33038b01000be001e000000b0009'
generated=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
assert 'bool swarmAllDead' not in generated and 'lagiUpdateGarilDualCompletion(pThis);' in generated
assert generated.index('lagiUpdateGarilDualCompletion(pThis);')<generated.index('lagiUpdateGarilDualSpacing(pThis);')
print('PASS 32768 mixed death-count/actor-retirement cases, shared cleanup, native count predicate, quadrant order and turn release')
