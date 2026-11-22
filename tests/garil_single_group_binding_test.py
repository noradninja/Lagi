from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'src/integration/lagi_garil_single_init.inc').read_text();motion=s[s.index('// Original 0605C2BC'):s.index('void createExplosionAtPosition')]
f=(r/'tests/garil_variant_burst_test.cpp').read_text();f=f[:f.index('struct s_workAreaCopy')]
f=f.replace('s32 value=0;','s32 m_value=0;').replace(':value(v)',':m_value(v)').replace('return value;','return m_value;')
f+=r'''
#include "lagi/garil_actor_group.h"
struct Block{sVec3_FP m0_position;};
struct sEntityGroup{Block* m4_positionBlock=nullptr;int mF_deadCount=0;};
struct sBTL_A5_SingleEnemy{LagiGarilSingleGroupBinding m_lagiGroup;sVec3_FP m60_targetPosition,m90_initialPosition,m48_scaledAutoScroll,m30_position;int m1C_entityIndex=0,mA8_soundEffectId=0x51E;};
struct Positions{std::array<sVec3_FP,3> values;};
// MOTION
int main(){
 std::array<Block,2> blocks;std::array<sEntityGroup,2> groups;std::array<Positions,2> positions;std::array<sBTL_A5_SingleEnemy,2> actors;std::array<u8,2> active={0xA5,0x5A},reaction={0xAA,0x55};std::array<std::array<u8,12>,2> dead{};
 for(unsigned i=0;i<2;++i){groups[i].m4_positionBlock=&blocks[i];actors[i].m1C_entityIndex=i;actors[i].m_lagiGroup={&groups[i],&active[i],&reaction[i],dead[i].data(),&positions[i],[](void* p,int index)->sVec3_FP{return static_cast<Positions*>(p)->values[index];}};}
 for(unsigned tick=0;tick<12000;++tick)for(unsigned i=0;i<2;++i){
  auto& a=actors[i];auto& target=positions[i].values[i];for(unsigned axis=0;axis<3;++axis){target[axis]=s32((tick+1)*(i+3)*(axis+7)*11);blocks[i].m0_position[axis]=s32(i*100000+tick*7+axis*13);}
  std::array<s32,3> expectedCurrent,expectedVelocity;
  for(unsigned axis=0;axis<3;++axis){const auto delta=u32(target[axis].asS32())-u32(a.m90_initialPosition[axis].asS32());s32 signedDelta;std::memcpy(&signedDelta,&delta,4);expectedVelocity[axis]=s32((int64_t(signedDelta)*a.mA8_soundEffectId)>>16);expectedCurrent[axis]=a.m90_initialPosition[axis].asS32()+expectedVelocity[axis];}
  lagiUpdateGarilSinglePosition(&a);assert(a.m_lagiGroup.group==&groups[i]);
  for(unsigned axis=0;axis<3;++axis){assert(a.m90_initialPosition[axis].asS32()==expectedCurrent[axis]);assert(a.m48_scaledAutoScroll[axis].asS32()==expectedVelocity[axis]);assert(a.m30_position[axis].asS32()==expectedCurrent[axis]+blocks[i].m0_position[axis].asS32());}
 }
 *actors[0].m_lagiGroup.activeMask|=2;*actors[0].m_lagiGroup.reactionMask&=~2;actors[0].m_lagiGroup.group->mF_deadCount++;const s32 retired=1;std::memcpy(actors[0].m_lagiGroup.deadWords,&retired,4);
 assert(active[1]==0x5A&&reaction[1]==0x55&&groups[1].mF_deadCount==0);for(auto v:dead[1])assert(!v);
 puts("PASS explicit single group binding: two independent live layouts/centers over 24000 actor ticks, separate flags/dead storage and counters");
}
'''
f=f.replace('// MOTION',motion)
with tempfile.TemporaryDirectory(prefix='lagi-single-group-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_single.cpp').read_text()
u=s[s.index('static bool BTL_A5_singleEnemy_applyAutoScrollAndCheckDistance'):s.index('// 0605bbd0')]
assert 'm158_formation->' not in u
for name in ['activeMask','reactionMask','deadWords','group']:
 assert 'm_lagiGroup.'+name in u
assert 'pEntity->m_lagiGroup={&pParent->mE0_entityGroup' in s
print('PASS generated single actor movement/lifecycle uses explicit binding')
