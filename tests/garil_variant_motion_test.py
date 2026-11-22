from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
base=(r/'tests/garil_variant_burst_test.cpp').read_text()
base=base[:base.index('struct s_workAreaCopy')]
fixture=base+r'''
struct Formation{struct{sVec3_FP m0_position;}m2C_posBlock;}formation;
struct sBTL_A5_VariantEnemy{Formation* m158_formation=&formation;sVec3_FP m60_targetPosition,m90_initialPosition,m48,m30_position;int mA8_soundEffectId=0x1999;fixedPoint m_lagiAltitude;};
#include "motion.inc"
s32 signedBits(u32 v){s32 out;std::memcpy(&out,&v,4);return out;}
int main(){
 sBTL_A5_VariantEnemy actor;std::array<s32,3> current={INT32_MAX,-1234567,INT32_MIN};actor.m_lagiAltitude=-9876;
 for(unsigned axis=0;axis<3;++axis)actor.m90_initialPosition[axis]=current[axis];
 u32 random=0x12345678;
 for(unsigned tick=0;tick<12000;++tick){
  std::array<s32,3> expectedWorld,expectedVelocity;
  actor.mA8_soundEffectId=tick%5==0?0:tick%5==1?0x10000:0x1999;
  for(unsigned axis=0;axis<3;++axis){
   random=random*1664525u+1013904223u;const s32 target=signedBits(random);actor.m60_targetPosition[axis]=target;
   random=random*1664525u+1013904223u;const s32 center=signedBits(random);formation.m2C_posBlock.m0_position[axis]=center;
   const s32 delta=signedBits(u32(target)-u32(current[axis]));
   const int64_t product=int64_t(delta)*actor.mA8_soundEffectId;
   expectedVelocity[axis]=signedBits(u32(uint64_t(product)>>16));
   current[axis]=signedBits(u32(current[axis])+u32(expectedVelocity[axis]));
   expectedWorld[axis]=signedBits(u32(center)+u32(current[axis]));
  }
  lagiUpdateGarilVariantPosition(&actor);
  for(unsigned axis=0;axis<3;++axis){assert(actor.m48[axis].asS32()==expectedVelocity[axis]);assert(actor.m90_initialPosition[axis].asS32()==current[axis]);assert(actor.m30_position[axis].asS32()==(axis==1?-9876:expectedWorld[axis]));}
 }
 puts("PASS 12000-frame native variant positional recurrence, separate altitude, rate edges and wrap");
}
'''
with tempfile.TemporaryDirectory(prefix='lagi-variant-motion-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture);(p/'motion.inc').write_text((r/'src/integration/lagi_garil_variant_motion.inc').read_text())
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
assert s.index('lagiUpdateGarilVariantPosition(pThis);')<s.index('lagiUpdateGarilVariantMembership(pThis);')<s.index('    // damage check')
assert '// auto-scroll position' not in s
assert 'fixedPoint m_lagiAltitude;' in s
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value in [(0x605563C,0x158),(0x605563E,0x1C8),(0x6055558,0x1E2D),(0x605504C,0x1999),(0x60A23C2,0xE060),(0x60A23E6,0xE078)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Native movement contract changed at {ea:08X}'
print('PASS native movement operands and generated execution order')
