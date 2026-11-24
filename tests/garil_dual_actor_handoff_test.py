from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
f=(r/'tests/garil_dual_actor_handoff_test.cpp').read_text()
a=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_single.cpp').read_text()
a=a[a.index('static void BTL_A5_singleEnemy_Update'):]
f=f.replace('// ACTOR_STAGES',a[a.index('    case 0:'):a.index('    case 2:')])
a=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
f=f.replace('// FORMATION_PHASES',a[a.index('    case 6:'):a.index('    case 10:')])
f=f.replace('// PRODUCERS',(r/'src/integration/lagi_garil_single_attack.inc').read_text())
f=f.replace('// TIMER',(r/'src/integration/lagi_garil_dual_control.inc').read_text().split('void battleEngine_PlayAttackCamera(int);')[0])
with tempfile.TemporaryDirectory(prefix='lagi-dual-actor-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),'/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
