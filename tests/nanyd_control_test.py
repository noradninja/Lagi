from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_formation.cpp').read_text()
u=s[s.index('static void BTL_A5_formationTask_Update'):]
gate=u[u.index('    case 3:'):u.index('    case 4:')]
phases=u[u.index('    case 5:'):u.index('    case 9:')]
assert 'lagiSelectNanydAttack(pThis,pEngine)' in gate and 'lagiTransitionFormationActors' in phases
v=(r/'build-vita/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp').read_text()
a=v.index('void arachnoth_startDragonEvade(');b=v.index('\n}\n',a)+3
fixture=(r/'tests/nanyd_control_test.cpp').read_text()
for token,text in [('EVADE',v[a:b]),('SCATTER',(r/'src/integration/lagi_a5_scatter_formation.inc').read_text()),('CONTROL',(r/'src/integration/lagi_nanyd_control.inc').read_text()),('GATE',gate),('PHASES',phases)]:
 fixture=fixture.replace('// PRODUCTION_'+token,text)
with tempfile.TemporaryDirectory(prefix='lagi-nanyd-control-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
if len(sys.argv)>1:
 data=Path(sys.argv[1]).read_bytes();base=0x06054000
 anchors={0x0605D35C:0x060A28FE,0x0605D360:0x060A28A4,0x0605D36C:0x060A29E8,
  0x0605D370:0x060B0E14,0x0605D374:0x060B0F90,0x0605D378:0x0606196C,0x0605D13C:0x060B0D24}
 for ea,value in anchors.items():assert int.from_bytes(data[ea-base:ea-base+4],'big')==value,f'Native selector contract differs at {ea:08X}'
 for ea,value in [(0x0605D00A,0xE30F),(0x0605D06E,0xE33C)]:assert int.from_bytes(data[ea-base:ea-base+2],'big')==value
 print('PASS original native Nanyd actor-selection helpers, mode-A entry, variant/restore tables and completion thresholds')
