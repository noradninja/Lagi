"""Execute the shared defeat notification with Azel's real meter update.
Run from an MSVC developer shell: python tests/battle_enemy_lifecycle_test.py
"""
from pathlib import Path
import subprocess, tempfile
r = Path(__file__).resolve().parents[1]
s = (r / 'extern/Azel/AzelLib/battle/battleEnemyLifeMeter.cpp').read_text()
a = s.index('void createEnemyLifeMeterTask_update(')
b = s.index('const std::array<u16, 10>', a)
fixture = (r / 'tests/battle_enemy_lifecycle_test.cpp').read_text()
fixture = fixture.replace('// PRODUCTION_UPDATE', s[a:b])
fixture = fixture.replace('// PRODUCTION_NOTIFICATION', (r / 'src/integration/lagi_battle_enemy_lifecycle.h').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-lifecycle-') as d:
    p = Path(d)
    (p / 'test.cpp').write_text(fixture)
    subprocess.run(['cl', '/nologo', '/EHsc', '/std:c++20', str(p/'test.cpp'), '/Fe:'+str(p/'test.exe'), '/Fo:'+str(p/'test.obj')], check=True)
    subprocess.run([str(p/'test.exe')], check=True)
