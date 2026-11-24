from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'src/integration/lagi_garil_story_worm.cpp').read_text()
a=s.index('struct sLagiA5WormTracker:');b=s.index('void lagiTrackA5StoryWorm(',a)
fixture=(r/'tests/garil_worm_tracker_test.cpp').read_text().replace('// PRODUCTION_TRACKER',s[a:b])
with tempfile.TemporaryDirectory(prefix='lagi-worm-tracker-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
