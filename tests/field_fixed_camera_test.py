"""Exercise production fixed-position tracking used by authored passage entry."""
from pathlib import Path
import re,subprocess,sys,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'extern/Azel/AzelLib/field/fieldCamera.cpp').read_text();c=(r/'CMakeLists.txt').read_text()
a=s.index('static void cameraFollowMode_scriptTarget(sFieldCameraStatus* r4)\n{');b=s.index('// 06062ac0',a)
function=s[a:b]
def binding(name):return re.search(r'set\('+name+r' \[=\[(.*?)\]=\]\)',c,re.S).group(1)
old,new=binding('fixed_camera_stub'),binding('fixed_camera_tracking');assert function.count(old)==1
function=function.replace(old,new)
f=(r/'tests/field_fixed_camera_test.cpp').read_text().replace('// PRODUCTION_CAMERA',function)
with tempfile.TemporaryDirectory(prefix='lagi-fixed-camera-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
p=Path(sys.argv[1]).read_bytes()
for ea,v,n in [(0x6056D7C,0xE400,2),(0x609E8E8,0x606A4F4,4),
 (0x606A538,0xA08E,2),(0x606A658,0x63E2,2),(0x606A664,0x3238,2),
 (0x606A68C,0x1E23,2),(0x606A692,0x1E34,2),(0x606A698,0x64E3,2),
 (0x606A69A,0x1E09,2)]:
 o=ea-0x6054000;assert int.from_bytes(p[o:o+n],'big')==v,f'Native fixed camera differs at {ea:08X}'
print('PASS original passage mode 0, fixed-camera table, unscripted branch, camera-to-dragon delta and pitch/yaw/distance stores')
