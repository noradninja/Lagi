from pathlib import Path
import subprocess,sys,tempfile
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2,'Pass original BTL_A5_3.PRG'
b=Path(sys.argv[1]).read_bytes();base=0x06054000
u=lambda ea,n=2:int.from_bytes(b[ea-base:ea-base+n],'big')
# Native creation uses hardware reciprocal, then signed fixed-point multiply;
# Update adds before decrementing the signed short and marking the task done.
for ea,value,n in [(0x060540C2,0x85E8,2),(0x060540C4,0x70FF,2),
 (0x060540C6,0x81E8,2),(0x060540C8,0x600F,2),(0x060540CA,0x4015,2),
 (0x06054136,0x4A28,2),(0x0605419C,0x0602FC72,4),
 (0x060541A0,0x06035644,4),(0x060541A4,0x06035664,4),
 (0x060541A8,0x10000,4),(0x060550D2,180,2),(0x06054E56,270,2)]:
 assert u(ea,n)==value,f'Original transition contract differs at {ea:08X}'
with tempfile.TemporaryDirectory(prefix='lagi-vector-transition-') as tmp:
 d=Path(tmp)
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(r/'include'),str(r/'tests/native_vector_transition_test.cpp'),'/Fe:'+str(d/'test.exe'),'/Fo:'+str(d/'test.obj')],check=True)
 subprocess.run([str(d/'test.exe')],check=True)
print('PASS original Lathum vector-task instruction and duration contracts')
