from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=(r/'tests/oasis_monolith_activation_test.cpp').read_text().replace('// SETTLED',(r/'src/integration/lagi_oasis_monolith_settled.inc').read_text()).replace('// ACTIVATION',(r/'src/integration/lagi_oasis_monolith_activation.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-oasis-activation-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
b=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605AD1C,0x605B694,4),(0x605B750,0x5BA,2),(0x605B6E4,0xE405,2),(0x605B76C,0x602439E,4),(0x605B752,0x333,2),(0x605B77C,0x16C16C,4),(0x605B788,0x608C8AC,4),(0x605B784,0x605B58A,4),(0x605B172,0xE278,2),(0x605B1A2,0xE570,2),(0x605B1BA,0xE473,2),(0x605B1E0,0xF0,2)]:
 assert int.from_bytes(b[ea-base:ea-base+n],'big')==value,f'Native activation operand changed at {ea:08X}'
assert int.from_bytes(b[0x6098C32-base:0x6098C34-base],'big')==0x6E4
s=(r/'build-vita/azel_upstream/field_field_a5_a5_skyEntity.cpp').read_text()
assert 'reinterpret_cast<void*>(&lagiOasisMonolithActivate)' in s
print('PASS original destruction callback, script/camera/timing operands and hole-under-monolith story gate')
