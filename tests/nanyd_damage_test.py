from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='lagi-nanyd-damage-') as d:
 p=Path(d);(p/'nanyd.inc').write_text((r/'src/integration/lagi_nanyd_damage.inc').read_text());(p/'shared.inc').write_text((r/'src/integration/lagi_battle_model_damage.inc').read_text())
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(p),'/I'+str(r/'include'),str(r/'tests/nanyd_damage_test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_enemy.cpp').read_text()
assert 'pThis->mB2_health -= t.m58;' not in s and 'static bool BTL_A5_enemy_checkDamage' in s
assert 'lagiUpdateBattleEnemyModelDamage({actor,actor->m14_flags,actor->mB2_health,' in s
assert 'lagiA5ScatterRetireIfComplete(pThis)' in s and 'lagiBattleEnemyDefeated(pThis->mAC_lifeMeter)' in s
b=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value in [(0x60A38DA,0xB2),(0x60A38DC,0xB4),(0x60A38DE,0xAC),(0x60A38E0,0xB6),(0x60A39F4,0x144),(0x60A39F8,0x14C),(0x60A39FA,0xB8),(0x60A39FC,0xBC),(0x60A39FE,0xC0),(0x60A3A00,0xC4),(0x60A3B0E,0x1000)]:
 assert int.from_bytes(b[ea-base:ea-base+2],'big')==value,f'Native shared damage contract differs at {ea:08X}'
print('PASS original native damage fields/callback/display-window operands and retained Scatter/death integration')
