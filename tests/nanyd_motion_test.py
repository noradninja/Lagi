from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2,'Pass original BTL_A5.PRG'
actor=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_enemy.cpp').read_text()
formation=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_formation.cpp').read_text()
assert 'lagiUpdateNanydPosition(pThis);' in actor and 'move 1/4 of the way each frame' not in actor
assert formation.index('lagiUpdateNanydFormationTrail(pThis,pEngine);')<formation.index('lagiA5BeginQueenScatter(pThis, pEngine);')
fixture=(r/'tests/nanyd_motion_test.cpp').read_text()
fixture=fixture.replace('// PRODUCTION_TRAIL',(r/'src/integration/lagi_nanyd_formation_trail.inc').read_text())
fixture=fixture.replace('// PRODUCTION_MOTION',(r/'src/integration/lagi_nanyd_motion.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-nanyd-motion-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe'),sys.argv[1]],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x06054000
for ea,value in [(0x0605D1B2,0xE710),(0x0605D246,0x00F4),(0x0605D248,0x00C0),(0x0605D1EE,0x76F4),(0x0605EB6C,0x0FFF),(0x0605EB6E,0x00FF)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Original motion contract differs at {ea:08X}'
assert list(data[0x060B0F04-base:0x060B0F04-base+20])==[0,3,3,3,3,6,6,6,6,6,9,9,9,9,12,12,12,12,12,15]
print('PASS original native trail count/order, spring constants and full swarm history-index map')
