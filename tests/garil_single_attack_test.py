from pathlib import Path
import subprocess,sys,tempfile
r=Path(__file__).resolve().parents[1]
generated=r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_single.cpp'
s=generated.read_text()
u=s[s.index('static void BTL_A5_singleEnemy_Update'):]
stages=u[u.index('    case 0:'):u.index('    case 2:')]
assert 'lagiGarilSingleBeginCharge(pThis);' in stages
assert 'lagiGarilSingleFire(pThis);' in stages
assert 'Unimplemented()' not in stages
fixture=(r/'tests/garil_single_attack_test.cpp').read_text()
fixture=fixture.replace('// PRODUCTION_PRODUCERS',(r/'src/integration/lagi_garil_single_attack.inc').read_text())
fixture=fixture.replace('// PRODUCTION_STAGES',stages)
with tempfile.TemporaryDirectory(prefix='lagi-single-attacks-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
if len(sys.argv)>1:
 data=Path(sys.argv[1]).read_bytes();base=0x06054000
 anchors={0x0605C07E:(0x01B8,2),0x0605C080:(0x00C8,2),
  0x0605C090:(0x060AFE44,4),0x0605C094:(0x060AFAFC,4),
  0x0605C0A4:(0x06054080,4),0x0605BC9C:(0x01B8,2),
  0x060540BE:(0xE24B,2),0x060540CA:(0xC909,2),
  0x0605410C:(0xE221,2),0x06054112:(0xE308,2),0x0605412A:(0xE318,2),
  0x0605C622:(0x0333,2),0x0605C624:(0x4000,2),0x0605C626:(0x2000,2)}
 for ea,(expected,size) in anchors.items():
  assert int.from_bytes(data[ea-base:ea-base+size],'big')==expected,f'Original attack contract changed at {ea:08X}'
 print('PASS original Saturn role, target index, quads, delay, damage and charge constants')
