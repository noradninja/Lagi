from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
base=(r/'tests/garil_variant_burst_test.cpp').read_text()
base=base[:base.index('struct s_workAreaCopy')]
fixture=base+r'''
struct Formation{struct{sVec3_FP m0_position;u8 mC_pad[12]{};}m2C_posBlock;std::array<u8,3> mF4_entityPositions{};}formation;
struct sBTL_A5_VariantEnemy{Formation* m158_formation=&formation;sVec3_FP m60_targetPosition,m90_initialPosition,m48,m30_position;int mA8_soundEffectId=0x1999;fixedPoint m_lagiAltitude;sVec3_FP m84_displayRotation,m9C_targetRotation,m3C_rotation,m_lagiAngularVelocity;int m1C_entityIndex=0;};
struct File{u32 getSaturnPtr(u32 a){return a;}}file;auto* g_BTL_A5=&file;
s32 readSaturnS32(u32 a){assert(a>=0x60AD5A4&&a<=0x60AD5B0);return s32(((a-0x60AD5A4)/4)*0x04000000);}
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
 for(unsigned quadrant=0;quadrant<4;++quadrant){
  actor.m1C_entityIndex=quadrant%3;formation.mF4_entityPositions[actor.m1C_entityIndex]=quadrant;
  sVec3_FP parent={-111,222,-333};std::memcpy(formation.m2C_posBlock.mC_pad,&parent,12);
  actor.m84_displayRotation={-0x1234567,0,0x3456789};actor.m9C_targetRotation={0x2222222,0x0C000000,-0x7654321};actor.mA8_soundEffectId=0x1999;
  std::array<s32,3> expected={0x2222222,0x0C000000,-0x7654321};
  for(unsigned tick=0;tick<3000;++tick){
   for(unsigned axis=0;axis<3;++axis){const s32 target=axis==1?s32(quadrant*0x04000000):actor.m84_displayRotation[axis].asS32();const s32 delta=signedBits(u32(target)-u32(expected[axis]));const auto velocity=signedBits(u32(uint64_t(int64_t(delta)*0x1999)>>16));expected[axis]=signedBits(u32(expected[axis])+u32(velocity));}
   lagiUpdateGarilVariantRotation(&actor);
   for(unsigned axis=0;axis<3;++axis){assert(actor.m9C_targetRotation[axis].asS32()==expected[axis]);assert(actor.m3C_rotation[axis].asS32()==signedBits(u32(parent[axis].asS32())+u32(expected[axis])));}
  }
 }
 puts("PASS 12000-frame rotation across every quadrant, nonzero parent rotation and native long-way interpolation");
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
assert s.index('lagiUpdateGarilVariantPosition(pThis);')<s.index('lagiUpdateGarilVariantRotation(pThis);')<s.index('lagiUpdateGarilVariantMembership(pThis);')
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value in [(0x605563C,0x158),(0x605563E,0x1C8),(0x6055558,0x1E2D),(0x605504C,0x1999),(0x60A23C2,0xE060),(0x60A23E6,0xE078)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Native movement contract changed at {ea:08X}'
print('PASS native movement operands and generated execution order')
for i in range(4):
 ea=0x60AD5A4+i*4
 assert int.from_bytes(data[ea-base:ea-base+4],'big')==i*0x04000000
print('PASS original variant rotation table')
