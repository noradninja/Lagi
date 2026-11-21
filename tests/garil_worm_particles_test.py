"""Run the production worm producers and story emission loop against recording services."""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'src/integration/lagi_garil_story_worm.cpp').read_text()
a=s.index('        // Native m110');b=s.index('    static void Draw',a)
loop=s[a:b].rsplit('    }',1)[0]
sand=(r/'src/integration/lagi_garil_worm_particles.inc').read_text()
fixture=(r/'tests/garil_worm_particles_test.cpp').read_text()
fixture=fixture.replace('// PRODUCTION_PRODUCERS',sand).replace('// PRODUCTION_LOOP',loop)
with tempfile.TemporaryDirectory(prefix='lagi-worm-particles-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
