"""Production A5 wind pointer-table lookup/emission against the original PRG."""
from pathlib import Path
import re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'extern/Azel/AzelLib/field/field_a5/a5_exitEntity.cpp').read_text()
cmake=(root/'CMakeLists.txt').read_text()
for name in ['a5_wind_table','a5_wind_mode0','a5_wind_mode1_table','a5_wind_mode1','a5_wind_mode3_table','a5_wind_mode3','a5_wind_rebind','a5_wind_gravity']:
 def binding(suffix):
  m=re.search(r'set\('+name+suffix+r' (?:\[=\[(.*?)\]=\]|"(.*?)")\)',cmake,re.S)
  return m[1] if m[1] is not None else m[2]
 old,new=binding('_old'),binding('_new');assert old in source
 source=source.replace(old,new)
def function(s,name):
 a=s.index(name);a=s.rfind('\n',0,a)+1;b=s.index('{',a);depth=1;c=b+1
 while depth:depth+=(s[c]=='{')-(s[c]=='}');c+=1
 return s[a:c]
quad=(root/'src/integration/lagi_vdp1_animated_quad_vita.cpp').read_text()
parts=[function(quad,'std::vector<sVdp1Quad> initVdp1Quad'),
       function(source,'static const std::vector<sVdp1Quad>* a5GetOrParseQuadList'),
       (root/'src/integration/lagi_a5_wind_quad_table.inc').read_text()]
for name in ['spawnFallingParticle','spawnDirectionalParticle','spawnWormParticle','Mode0','Mode3']:
 parts.append(function(source,'static void a5ExitEntity_'+name+'('))
fixture=(root/'tests/a5_wind_particle_test.cpp').read_text().replace('// PRODUCTION_WIND','\n'.join(parts))
with tempfile.TemporaryDirectory(prefix='lagi-a5-wind-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(root/'extern/Azel/AzelLib'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe'),sys.argv[1]],check=True)
p=Path(sys.argv[1]).read_bytes()
for ea,value,n in [(0x6058868,0x6099804,4),(0x6058800,0xC903,2),
 (0x6058802,0x4008,2),(0x6058804,0x031E,2),(0x60589D8,0x6099814,4),
 (0x6058C76,0x2432,2),(0x6058D1A,0x2432,2),(0x6058DF6,0x2E22,2),
 (0x6058D06,0xE251,2),(0x6058D0C,0x1429,2)]:
 o=ea-0x6054000;assert int.from_bytes(p[o:o+n],'big')==value,f'Native wind contract differs at {ea:08X}'
print('PASS original indexed pointer load, per-slot quad rebind and worm sand gravity instructions')
