"""Run production generated camera Update with moving and fixed subjects.
Requires MSVC developer shell and a configured native build directory.
"""
from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
build=Path(sys.argv[1]) if len(sys.argv)>1 else r/'build-vita'
s=(build/'azel_upstream/field_fieldCutsceneTask2.cpp').read_text()
a=s.index('void s_cutsceneTask2::Update(s_cutsceneTask2* pThis)')
b=s.index('void s_cutsceneTask2::Draw',a)
fixture=(r/'tests/field_camera_tracking_test.cpp').read_text().replace('// PRODUCTION_UPDATE',s[a:b])
with tempfile.TemporaryDirectory(prefix='lagi-camera-tracking-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
