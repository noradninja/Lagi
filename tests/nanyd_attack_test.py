from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2,'Pass original BTL_A5.PRG'
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_enemy.cpp').read_text()
assert 'if(lagiUpdateNanydAttackState(pThis))return;' in s and 'lagiNanydEnterPendingAttack(pThis);' in s
assert 'if (pThis->m28_state < 7)' in s
fixture=(r/'tests/nanyd_motion_test.cpp').read_text().split('static sVec3_FP headAt',1)[0]
fixture=fixture.replace('// PRODUCTION_TRAIL',(r/'src/integration/lagi_nanyd_formation_trail.inc').read_text())
fixture=fixture.replace('// PRODUCTION_MOTION',(r/'src/integration/lagi_nanyd_motion.inc').read_text())
fixture+=(r/'tests/nanyd_attack_cases.inc').read_text().replace('// PRODUCTION_ATTACKS',(r/'src/integration/lagi_nanyd_attacks.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-nanyd-attacks-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),'/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe'),sys.argv[1]],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x06054000
for ea,value in [(0x0605ECD8,0xB5EE),(0x0605ECDA,0x4A12),(0x0605EE6A,0xFF),(0x0605EE6C,0x2000),(0x0605EE70,0x3000),(0x0605EE72,0x800),(0x0605EE9E,0xE245),(0x0605EFCE,0x3000),(0x0605EFD0,0x800),(0x0605EFA2,0xE25A)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Original attack contract differs at {ea:08X}'
assert int.from_bytes(data[0x0605EFD8-base:0x0605EFD8-base+4],'big')==0x14000
print('PASS original native rush coefficients, RNG mask, collision radii, damage flags and completion thresholds')
