"""Exercise the production shared pool allocator/update with the CMake adapter.
Run from a VS developer shell: python tests/field_particle_lifetime_test.py [FLD_A5.PRG]
"""
from pathlib import Path
import re, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'extern/Azel/AzelLib/field/fieldParticlePool.cpp').read_text()
cmake=(root/'CMakeLists.txt').read_text()
def binding(name):
 return re.search(r'set\('+name+r' \[=\[(.*?)\]=\]\)',cmake,re.S).group(1)
old,new=binding('pool_payload_old'),binding('pool_payload_new')
assert source.count(old)==1
source=source.replace(old,new)
def function(name):
 start=source.index(name); start=source.rfind('\n',0,start)+1
 first=source.index('{',start); depth=1; end=first+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
fixture=(root/'tests/field_particle_lifetime_test.cpp').read_text()
fixture=fixture.replace('// PRODUCTION_POOL_UPDATE',function('static void particlePoolUpdate'))
fixture=fixture.replace('// PRODUCTION_POOL_SPAWN',function('s32 spawnParticleInPool'))
with tempfile.TemporaryDirectory(prefix='lagi-field-pool-') as directory:
 p=Path(directory); (p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/W4',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
if len(sys.argv)>1:
 p=Path(sys.argv[1]).read_bytes()
 def value(ea,n=4):return int.from_bytes(p[ea-0x6054000:ea-0x6054000+n],'big')
 # Finished slot -> payload non-null -> freeHeapForTask(pool,payload) -> clear.
 for ea,v,n in [(0x6080450,0x430B,2),(0x6080454,0x2008,2),
                (0x6080458,0x53D6,2),(0x6080464,0x55E8,2),
                (0x6080466,0x4A0B,2),(0x6080468,0x64D3,2),
                (0x608046A,0x1EC8,2),(0x60804CC,0x60151FE,4)]:
  assert value(ea,n)==v,f'Native pool cleanup contract differs at {ea:08X}'
 print('PASS original A5 payload-free call and arguments')
