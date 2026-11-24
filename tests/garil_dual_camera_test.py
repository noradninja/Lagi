from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=(r/'tests/garil_variant_burst_test.cpp').read_text();f=f[:f.index('struct s_workAreaCopy')]
f+=r'''
struct sBTL_A5_DualFormation{sVec3_FP mA8_position,mC0_position2;struct{sVec3_FP m0_position;}m2C_posBlock;s32 m280_phase=0;};
struct s_battleEngine{sVec3_FP m1A0_battleAutoScrollDelta;};
// CAMERA
s32 signedBits(u32 value){s32 result;std::memcpy(&result,&value,4);return result;}
int main(){
 sBTL_A5_DualFormation f;s_battleEngine e;u32 random=0xDEADBEEF;
 const std::array<s32,5> phases={0,1,-1,2,INT32_MAX};
 for(unsigned tick=0;tick<60000;++tick){f.m280_phase=phases[tick%5];std::array<s32,3> eye,target;
  for(unsigned axis=0;axis<3;++axis){random=random*1664525u+1013904223u;e.m1A0_battleAutoScrollDelta[axis]=signedBits(random);random=random*1664525u+1013904223u;f.m2C_posBlock.m0_position[axis]=signedBits(random);eye[axis]=signedBits(u32(f.mA8_position[axis].asS32())+u32(e.m1A0_battleAutoScrollDelta[axis].asS32()));target[axis]=f.m280_phase? signedBits(u32(f.mC0_position2[axis].asS32())+u32(e.m1A0_battleAutoScrollDelta[axis].asS32())):signedBits(u32(f.m2C_posBlock.m0_position[axis].asS32())-(axis==2?0x12000u:0u));}
  lagiUpdateGarilDualCamera(&f,&e);for(unsigned axis=0;axis<3;++axis){assert(f.mA8_position[axis].asS32()==eye[axis]);assert(f.mC0_position2[axis].asS32()==target[axis]);}
 }
 puts("PASS production dual camera: 60000 updates, phase switches, head tracking, both snapshot scrolls and full-width wrap");
}
'''.replace('// CAMERA',(r/'src/integration/lagi_garil_dual_camera.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-dual-camera-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
assert s.index('lagiUpdateGarilDualTrail(pThis);')<s.index('lagiUpdateGarilDualCamera(pThis,pEngine);')<s.index('    switch (pThis->m28_state)')
assert '// auto-scroll positions' not in s
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605F4F8,0x280,2),(0x605F4FC,0xA8,2),(0x605F500,0xAC,2),(0x605F504,0xB0,2),(0x605F520,0x12000,4),(0x605F5BE,0xC0,2),(0x605F5C0,0xC4,2),(0x605F5C2,0xC8,2)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Original camera contract changed at {ea:08X}'
print('PASS native phase/camera operands and generated trail-camera-state order')
