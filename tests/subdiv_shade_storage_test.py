"""Compare production cached shade writes with unconditional interpolation."""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1];s=(r/'src/platform/vita/neptune_renderer.cpp').read_text()
def function(name):
 a=s.index(name);a=s.rfind('\n',0,a)+1;b=s.index('{',a);end=b+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[a:end]
a=s.index('struct SubdivShadeState {');b=s.index('// Render-thread-only',a)
state=s[a:b]
prepare=s[s.index('bool prepare_vdp1_model('):s.index('static bool',s.index('bool prepare_vdp1_model(')+30)]
assert 'prepareSubdivShadeStorage(reuseGeometry);' in prepare
assert 'g_sceneSubdivShadeStates.clear()' not in prepare
rebuild=s[s.index('    if (g_liveTownStaticRebuilt) {'):s.index('    if (g_liveTownStaticRebuilt) {')+650]
assert 'g_sceneSubdivShadeStates.clear()' not in rebuild
# Keep native free/reset invalidation boundaries in place.
assert s.count('g_sceneSubdivShadeStates.clear();')==3
start=s.index('            std::array<std::uint32_t,12> shadeBits{};')
stop=s.index('            float gridX[9]',start)
calculate=s[start:stop]
a=s.index('            for (unsigned int i = 0; i < 9u; ++i) {\n                auto& dst =',stop)
b=s.index('            if (sceneStaticPositions &&',a)
write=s[a:b]
f=(r/'tests/subdiv_shade_storage_test.cpp').read_text().replace('// PRODUCTION_STATE',state).replace('// PRODUCTION_GRID',function('static inline void fillMidpointGrid3x3(')).replace('// PRODUCTION_CALCULATE',calculate).replace('// PRODUCTION_WRITE',write)
with tempfile.TemporaryDirectory(prefix='lagi-shade-storage-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/O2','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
