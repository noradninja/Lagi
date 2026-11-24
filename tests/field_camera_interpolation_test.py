"""Exercise production type-1 field camera interpolation and original A5 roll instructions."""
from pathlib import Path
import re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'extern/Azel/AzelLib/field/fieldCutsceneTask2.cpp').read_text()
cmake=(root/'CMakeLists.txt').read_text()
def binding(name):return re.search(r'set\('+name+r' \[=\[(.*?)\]=\]\)',cmake,re.S).group(1)
for name in ['camera_roll_step','camera_roll_axis']:
 old,new=binding(name+'_old'),binding(name+'_new');assert source.count(old)==1
 source=source.replace(old,new)
old=binding('camera_tracking_old');assert source.count(old)==1
source=source.replace(old,old[:old.index('            //')]+'            pThis->m14 = pThis->m28 + *pThis->m24;\n        }')
a=source.index('s32 s_cutsceneTask2::UpdateSub0()');b=source.index('void s_cutsceneTask2::Draw',a)
fixture=(root/'tests/field_camera_interpolation_test.cpp').read_text().replace('// PRODUCTION_CAMERA',source[a:b])
with tempfile.TemporaryDirectory(prefix='lagi-field-camera-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(root/'extern/Azel/AzelLib'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
if len(sys.argv)>1:
 p=Path(sys.argv[1]).read_bytes()
 for ea,v in [(0x6071BFA,0x536D),(0x6071BFC,0x556E),(0x6071BFE,0x3538),
              (0x6071C28,0xE15C),(0x6071C2C,0x2102),(0x6071D10,0x1035)]:
  o=ea-0x6054000;assert int.from_bytes(p[o:o+2],'big')==v,f'Native camera roll differs at {ea:08X}'
 print('PASS original A5 authored-roll operands, step storage and camera Z-axis write')
