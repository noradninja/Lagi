from pathlib import Path
import subprocess, tempfile, sys, hashlib
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2, 'Pass original Disc 1 FLD_A5.PRG'
p=Path(sys.argv[1]).read_bytes(); base=0x06054000
u=lambda ea,n=4:int.from_bytes(p[ea-base:ea-base+n], 'big')
for ea,expected,n in [(0x06054424,0x0608E34C,4),(0x06054428,0x0608E24C,4),
 (0x0605442C,0x06078D26,4),(0x0608D72C,0x06055AE0,4),
 (0x0608D730,0x06055D34,4),(0x0608D734,0x06055D9C,4),
 (0x06055D70,0x5B5,2),(0x06054DD2,0xE410,2),(0x06054DDC,0x90,2),
 (0x06054E30,0x060703E4,4),(0x06055D98,0x06054DCC,4),
 (0x06055D6E,0x51C,2),(0x0605619E,0xA000,2),
 (0x06055C4C,0x0604A40A,4),(0x06055C50,0x06042F14,4),
 (0x06055D6A,600,2),(0x06055D6C,240,2)]:
 assert u(ea,n)==expected,f'Original mound contract differs at {ea:08X}'
# COMMON identifies native encounter 0x10's remap as BTL_A5_3, not A5_2.
c=Path(sys.argv[1]).with_name('COMMON.DAT').read_bytes()
name=int.from_bytes(c[0x5DC+0x12*20+4:0x5DC+0x12*20+8],'big')-0x200000
assert c[name:c.index(0,name)]==b'BTL_A5_3.PRG'
# Exactly five original records, all belonging to the same cell/factory.
grid=u(0x0608E34C);rows=[]
for cell in range(4):
 q=u(grid+cell*4)
 while q and u(q):
  assert cell==1 and u(q)==0x06055E1E
  rows.append(q);q+=32
assert len(rows)==5
assert [u(u(q+28),2) for q in rows]==[0x1EC,0x1EC,0x1E8,0x1E8,0x1F0]
# Native transient model sequences each hold precisely 39 model/pose pairs.
assert [u(0x0608D474+i*4) for i in range(3)]==[0x0608D2A0,0x0608D33C,0x0608D3D8]
generated=r/'build-vita/azel_upstream'
assert 'lagiSetupGarilMoundField(pDT3, fieldA5_0_startTasks);' in (generated/'field_field_a5_o_fld_a5_0.cpp').read_text()
assert 'if (!lagiDispatchNativeFieldCellFactory(r4, r5[i], r6))' in (generated/'field.cpp').read_text()
assert 'void lagiApplyA5WindToParticle(sVec3_FP* p)' in (generated/'field_field_a5_a5_sandParticlePool.cpp').read_text()
source=(r/'src/integration/lagi_garil_mound.cpp').read_text()
source='\n'.join(line for line in source.split('\n') if not line.startswith('#include'))
registry=(r/'src/integration/lagi_native_field_cells.cpp').read_text()
registry='\n'.join(line for line in registry.split('\n') if not line.startswith('#include'))
with tempfile.TemporaryDirectory(prefix='lagi-garil-mound-') as d:
 d=Path(d);(d/'production.inc').write_text(source);(d/'registry.inc').write_text(registry)
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(r/'include'),'/I'+str(d),str(r/'tests/garil_mound_test.cpp'),'/Fe:'+str(d/'test.exe'),'/Fo:'+str(d/'test.obj')],check=True)
 subprocess.run([str(d/'test.exe'),sys.argv[1]],check=True)
 # Regression-test the existing shared Garil action adapter, including Lathum.
 action=(r/'src/integration/lagi_garil_script_native.cpp').read_text()
 action='\n'.join(line for line in action.split('\n') if not line.startswith('#include'))
 (d/'production.inc').write_text(action)
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(d),str(r/'tests/garil_script_native_test.cpp'),'/Fe:'+str(d/'actions.exe'),'/Fo:'+str(d/'actions.obj')],check=True)
 subprocess.run([str(d/'actions.exe')],check=True)

print('PASS original native setup operands, five cell objects, target/battle contract, animation tables and sand task descriptor; FLD_A5 SHA256',hashlib.sha256(p).hexdigest())
