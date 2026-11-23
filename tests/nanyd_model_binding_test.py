from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_enemy.cpp').read_text()
a=s.index('    s32 result = lagiCreateBattleEnemyModels');b=s.index('    initAnimation(',a);models=s[a:b]
a=s.index('    LagiBattleModelTargetState nativeTargets;');b=s.index('    pEntity->m1A6_timerMax',a);targets=s[a:b]
assert 'initBattleEnemyTargetables(pEntity' not in s
f=r"""
#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
#include "lagi/native_battle_models.h"
using s32=int32_t;using u8=uint8_t;
struct sSaturnPtr{unsigned ea;};struct File{sSaturnPtr getSaturnPtr(unsigned ea){return {ea};}}file;auto* g_BTL_A5=&file;
struct s_workAreaCopy{};struct s_fileBundle{};struct s_3dModel{};
struct Meter{bool retired=false;};
struct Enemy:s_workAreaCopy{s_fileBundle* m0_fileBundle;u8 m29_modelCount=0;std::vector<s_3dModel> m2C_models;unsigned m1C_entityIndex=0;Meter* mAC_lifeMeter;};
Enemy* expected;bool failModels=false,failTargets=false;unsigned modelCalls=0,targetCalls=0,lead=0;
bool lagiCreateBattleEnemyModels(s_workAreaCopy* owner,s_fileBundle* bundle,u8& count,std::vector<s_3dModel>& models,sSaturnPtr descriptors,u8 n){assert(owner==expected&&bundle==expected->m0_fileBundle&&&count==&expected->m29_modelCount&&&models==&expected->m2C_models&&n==1&&descriptors.ea==0x60B1130+lead*8);++modelCalls;return !failModels;}
bool lagiInitBattleEnemyModelTargets(s_workAreaCopy* owner,uint32_t index,std::vector<s_3dModel>& models,LagiBattleModelTargetState& state,const std::array<uint32_t,4>& callbacks){assert(owner==expected&&index==expected->m1C_entityIndex&&&models==&expected->m2C_models);assert((callbacks==std::array<uint32_t,4>{0x60A3A4C,0x60A3A50,0x60A3A74,0x60A3A98}));assert(state.auxiliaryCount==0&&!state.auxiliaryData);++targetCalls;return !failTargets;}
void lagiRetireBattleEnemyMeter(Meter* meter){meter->retired=true;}
Enemy* initModels(Enemy* pEntity,int iVar6){
// MODELS
 return pEntity;
}
Enemy* initTargets(Enemy* pEntity){
// TARGETS
 return pEntity;
}
int main(){
 for(unsigned index=0;index<20;++index)for(lead=0;lead<2;++lead)for(bool fail:{false,true}){
  s_fileBundle bundle;Meter meter;Enemy actor;actor.m0_fileBundle=&bundle;actor.mAC_lifeMeter=&meter;actor.m1C_entityIndex=index;expected=&actor;
  failModels=fail;modelCalls=0;assert(initModels(&actor,lead)==(fail?nullptr:&actor));assert(modelCalls==1&&meter.retired==fail);
  meter.retired=false;failTargets=fail;targetCalls=0;assert(initTargets(&actor)==(fail?nullptr:&actor));assert(targetCalls==1&&meter.retired==fail);
 }
 puts("PASS generated Nanyd model/target bindings: all 20 actor indices, both descriptors, exact shared fields/callbacks and failed-init meter retirement");
}
""".replace('// MODELS',models).replace('// TARGETS',targets)
with tempfile.TemporaryDirectory(prefix='lagi-nanyd-model-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
