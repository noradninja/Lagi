from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'src/integration/lagi_garil_single_init.inc').read_text()
s=s[:s.index('// Original 0605C2BC')]
fixture=(r/'tests/garil_single_init_test.cpp').read_text().replace('// PRODUCTION_INIT',s)
with tempfile.TemporaryDirectory(prefix='lagi-garil-init-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
if len(sys.argv)>1:
 ram=Path(sys.argv[1]).read_bytes()
 for ea,expected,n in [(0x605BC72,0x7570,2),(0x605BD0A,0x736C,2),(0x605BCB4,0x60B0028,4),
   (0x605B070,0x90,2),(0x605AFA6,0x7448,2),(0x605B078,0x60AF90C,4),
   (0x605C312,0x5551,2),(0x605C348,0x60A23C0,4)]:
  actual=int.from_bytes(ram[ea-0x6000000:ea-0x6000000+n],'big')
  assert actual==expected,f'Original constructor contract differs at {ea:08X}'
 print('PASS original Saturn constructor/position-table instruction anchors')
