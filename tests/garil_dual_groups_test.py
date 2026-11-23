from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
fixture=r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "lagi/garil_actor_group.h"
struct s_workArea{};struct s_workAreaCopy:s_workArea{};using p_workArea=s_workArea*;
struct sVec3_FP{int32_t v[3]{};int32_t& operator[](unsigned i){return v[i];}};
struct sSaturnPtr{uint32_t ea=0;sSaturnPtr operator+(unsigned n)const{return {ea+n};}};
struct sFormationPositionBlock{sVec3_FP m0_position,m30_target,m60_currentInterp;int m78_interpRate=0;};
struct sEntityGroup{int mC_formationNameIndex=-1;sFormationPositionBlock* m4_positionBlock=nullptr;sSaturnPtr m8_dataTable;};
struct sBTL_A5_DualFormation:s_workAreaCopy{sEntityGroup mE0_entityGroup,m1D0_entityGroup2;sFormationPositionBlock m2C_posBlock,m_lagiSecondaryBlock;uint8_t m1E0_pad5[3]{},m1E4_pad6[152]{},sentinel=0xA5;};
sVec3_FP readSaturnVec3(sSaturnPtr p){return {{int32_t(p.ea),22,33}};}
LagiGarilSingleGroupBinding captured;int calls=0;bool fail=false;s_workAreaCopy actor;
s_workArea* lagiCreateGarilSingleActor(s_workAreaCopy* parent,const LagiGarilSingleGroupBinding& binding,int index,int lead){assert(parent&&index>=0&&index<3&&lead==1);captured=binding;++calls;return fail?nullptr:&actor;}
// GROUPS
int main(){
 sBTL_A5_DualFormation dual;dual.m1D0_entityGroup2.m8_dataTable={0x60B13D8};
 lagiInitGarilDualGroups(&dual);assert(dual.mE0_entityGroup.mC_formationNameIndex==15&&dual.m1D0_entityGroup2.mC_formationNameIndex==17);assert(dual.mE0_entityGroup.m4_positionBlock==&dual.m2C_posBlock&&dual.m1D0_entityGroup2.m4_positionBlock==&dual.m_lagiSecondaryBlock);assert(dual.m_lagiSecondaryBlock.m0_position[1]==-0x13000&&dual.m_lagiSecondaryBlock.m30_target[1]==-0x13000&&dual.m_lagiSecondaryBlock.m60_currentInterp[1]==-0x13000);
 for(int i=0;i<3;++i){assert(lagiCreateGarilDualSingle(&dual,i)==&actor);assert(captured.group==&dual.m1D0_entityGroup2&&captured.activeMask==&dual.m1E0_pad5[0]&&captured.reactionMask==&dual.m1E0_pad5[2]&&captured.deadWords==dual.m1E4_pad6&&captured.positionContext==&dual);assert(captured.targetPosition(captured.positionContext,i)[0]==int32_t(0x60B13D8+i*12));}
 dual.m1D0_entityGroup2.m8_dataTable={0x60B13B4};assert(captured.targetPosition(captured.positionContext,2)[0]==0x60B13CC);fail=true;assert(!lagiCreateGarilDualSingle(&dual,0));
 lagiFinishGarilDualMotion(&dual);assert(dual.m2C_posBlock.m78_interpRate==0xA3D&&dual.m_lagiSecondaryBlock.m78_interpRate==0xA3D);assert(dual.m2C_posBlock.m0_position[1]==0x13000&&dual.m2C_posBlock.m30_target[1]==0x13000&&dual.m2C_posBlock.m60_currentInterp[1]==0x13000);assert(dual.m_lagiSecondaryBlock.m0_position[1]==0&&dual.m_lagiSecondaryBlock.m30_target[1]==0&&dual.m_lagiSecondaryBlock.m60_currentInterp[1]==0&&dual.sentinel==0xA5);
 puts("PASS production dual group bindings, authored records, correct actor factory, live second-table callback and constructor/factory motion overrides");
}
'''.replace('// GROUPS',(r/'src/integration/lagi_garil_dual_groups.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-dual-groups-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
assert 'BTL_A5_createEnemy(pThis, 0x060b13b4, i, 1)' not in s
assert 'lagiCreateGarilDualSingle(pThis,i)' in s
assert s.index('lagiInitGarilDualGroups(pThis);')<s.index('// swarm group: 10 enemies')
assert s.index('BTL_A5_dualFormation_Init(pTask);')<s.index('lagiFinishGarilDualMotion(pTask);')
assert 'formationUpdatePosition(&pThis->m_lagiSecondaryBlock,&pEngine->mC_battleCenter);' in s
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605F37E,0xA3D,2),(0x605F380,0xA4,2),(0x605F382,0x274,2),(0x605F3A0,0x13000,4),(0x605F384,0xFFFED000,4),(0x605F4F6,0x1FC,2)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Native dual motion contract changed at {ea:08X}'
print('PASS original final motion operands and generated dual constructor/update bindings')
