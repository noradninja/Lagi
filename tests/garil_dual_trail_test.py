from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
fixture=r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "lagi/native_formation_motion.h"
using u8=uint8_t;
struct sVec3_FP{int32_t v[3]{};};
struct sSaturnPtr{uint32_t ea=0;};struct File{sSaturnPtr getSaturnPtr(uint32_t a){return {a};}}file;auto* g_BTL_A5=&file;
struct sBTL_A5_DualFormation{struct{sSaturnPtr m8_dataTable;}mE0_entityGroup;sSaturnPtr m1CC_dataTable2;u8 m27C_variant=0;struct{sVec3_FP m0_position;}m2C_posBlock;u8 mF4_entityPositions[216]{};uint32_t sentinel=0xA5A5A5A5;};
// TRAIL
int main(){
 for(unsigned variant=0;variant<2;++variant){
  sBTL_A5_DualFormation f;f.m27C_variant=variant;f.m2C_posBlock.m0_position={{123,456,789}};std::array<sVec3_FP,18> expected;for(unsigned i=0;i<18;++i)expected[i]={{int(i),int(i*2),int(i*3)}};std::memcpy(f.mF4_entityPositions,expected.data(),216);
  lagiInitGarilDualSwarmLayout(&f);assert(f.mE0_entityGroup.m8_dataTable.ea==0x60B11C8+variant*0x78&&f.m1CC_dataTable2.ea==0x60B13A8);
  for(unsigned i=0;i<17;++i)expected[i]=f.m2C_posBlock.m0_position;assert(!std::memcmp(f.mF4_entityPositions,expected.data(),216));
  for(unsigned tick=0;tick<12000;++tick){
   f.m2C_posBlock.m0_position={{int(tick*11),-int(tick*7),int(tick*3+999)}};
   std::array<sVec3_FP,18> old=expected;expected[0]=f.m2C_posBlock.m0_position;expected[1]=expected[0];for(unsigned i=2;i<18;++i)expected[i]=old[i-1];
   lagiUpdateGarilDualTrail(&f);assert(!std::memcmp(f.mF4_entityPositions,expected.data(),216)&&f.sentinel==0xA5A5A5A5);
  }
 }
 puts("PASS production dual swarm layout and 24000 trail ticks: native 17-slot seed, duplicated head, reverse shift and bounded storage");
}
'''.replace('// TRAIL',(r/'src/integration/lagi_garil_dual_trail.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-dual-trail-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
assert s.index('lagiInitGarilDualSwarmLayout(pThis);')<s.index('// swarm group: 10 enemies')
assert s.index('formationUpdatePosition(&pThis->m_lagiSecondaryBlock')<s.index('lagiUpdateGarilDualTrail(pThis);')<s.index('    switch (pThis->m28_state)')
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605F18E,0xE610,2),(0x605F198,0xE378,2),(0x605F214,0xC0,2),(0x605F216,0xF4,2),(0x605FBF6,0xE610,2),(0x605FD00,0xF4,2),(0x605FD02,0xC0,2)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Original dual trail contract changed at {ea:08X}'
assert list(data[0x60B13A8-base:0x60B13A8-base+10])==[0,0,3,3,6,6,9,9,12,12]
print('PASS original dual seed/shift operands and actor trail mapping')
