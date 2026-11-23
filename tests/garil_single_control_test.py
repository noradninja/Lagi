from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_single.cpp').read_text()
u=s[s.index('static void BTL_A5_formationSingle_Update'):]
start=u.index('    case 3:');end=u.index('    case 4:',start)
gate=u[start:end].replace('if (battleEngine_isPlayerTurnActive()) return;','if (battleEngine_isPlayerTurnActive()) return;')
assert 'lagiSelectGarilSingleAttack(pThis)' in gate
fixture=(r/'tests/garil_single_control_test.cpp').read_text()
fixture=fixture.replace('// PRODUCTION_CONTROL',(r/'src/integration/lagi_garil_single_control.inc').read_text())
fixture=fixture.replace('// PRODUCTION_SELECTION_GATE',gate)
phases=u[u.index('    case 5:'):u.index('    case 0xB:')]
assert 'lagiTransitionFormationActors' in phases and 'battleEngine_restoreCameraAfterEnemyAttack' in phases
assert u.index('lagiUpdateGarilSingleSpacing(pThis);')<u.index('formationUpdatePosition(')
assert 'binding.targetPosition(binding.positionContext,entityIndex)' in s
fixture=fixture.replace('// PRODUCTION_PHASES',phases)
with tempfile.TemporaryDirectory(prefix='lagi-single-control-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),'/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
if len(sys.argv)>1:
 data=Path(sys.argv[1]).read_bytes();base=0x06054000
 anchors={0x0605BB60:(0x060A2A62,4),0x0605BB64:(0x060619AA,4),0x0605BB68:(0x06061CAE,4),
  0x0605B3DA:(0xE32D,2),0x0605B402:(0xE33C,2),0x0605B39C:(0x010D,2),
  0x060A2BDC:(0x04000000,4),0x060A2BE0:(0x08000000,4),0x060A2BE4:(0xFBFFFFFF,4),0x060A2BE8:(0x02000000,4),
  0x0605B890:(0x19000,4),0x0605B894:(0xFFFE7000,4),0x0605B898:(0x14000,4),0x0605B89C:(0xFFFEC000,4)}
 for ea,(expected,size) in anchors.items():
  assert int.from_bytes(data[ea-base:ea-base+size],'big')==expected,f'Original control contract changed at {ea:08X}'
 print('PASS original Saturn selector callees, actor flag transitions, timers and layout clamps')
