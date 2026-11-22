from pathlib import Path
import sys,subprocess,tempfile,hashlib
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2,'Pass original BTL_A5_3.PRG'
p=Path(sys.argv[1]);b=p.read_bytes();base=0x06054000
u=lambda ea,n=4:int.from_bytes(b[ea-base:ea-base+n],'big')
for ea,value,n in [(0x060565F2,0x18C,2),(0x06056608,0x060A4008,4),
 (0x060A4008,0x060542B0,4),(0x060543CC,0x060A3EC8,4),
 (0x060543D0,0x060A3EB4,4),(0x06054298,0x060A3EFB,4),
 (0x060542A0,0xF0004000,4),(0x060542A4,0xF0004002,4),
 (0x060542A8,0xA000,4),(0x0605436A,0xE60D,2),
 (0x06054370,0xE400,2),(0x06054372,0xE004,2),
 (0x060543BE,0x14C,2),(0x060543C0,0x138,2),
 (0x060543C4,0x06031EEA,4),(0x060543E0,0x06060614,4)]:
 assert u(ea,n)==value,f'Original Lathum initialization contract differs at {ea:08X}'
assert b[0x060A3EFB-base:0x060A3EFB-base+9]==bytes([1,2,1,2,1,2,1,1,2])
generated=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_3_data.cpp').read_text()
assert 'lagiInitializeLathumRig(pThis,pThis->rig)' in generated and 'LagiLathumRig rig;' in generated
assert 'lagiDrawLathumRig(pThis,pThis->rig)' in generated
assert list(b[0x060A3ED4-base:0x060A3ED4-base+9])==[8,3,9,10,6,0,4,5,7]
assert 'lagiUpdateLathumDamage(' in generated
assert list(b[0x060A3EDD-base:0x060A3EDD-base+9])==[0,0,0,1,1,2,2,3,3]
# Native damage-service operands and previous-camera selection; these are
# independent original-data checks rather than a second helper implementation.
for ea,value,n in [(0x06055C96,0x7658,2),(0x06055DB8,0x0604B060,4),
 (0x06055DD4,0x0605A61E,4),(0x06055DD8,0x0605B896,4),
 (0x06055DDC,0x40000,4),(0x06055DE0,0x0607068C,4),
 (0x06055DE8,0xFFF7FFFF,4),(0x06055DEC,0xFFFDFFFF,4),
 (0x06055EE8,0x0606960A,4),(0x06055EF4,0x0605A5B4,4),
 (0x06055E84,0xE701,2),(0x06055E86,0xE00B,2)]:
 assert u(ea,n)==value,f'Original Lathum damage contract differs at {ea:08X}'

assert list(b[0x060A3FFE-base:0x060A3FFE-base+9])==[15,4,16,19,9,2,5,8,12]
assert [u(0x060A3FC8+i*2,2) for i in range(27)]==[0xB0,0xB4,0xB8,0x98,0x9C,0xA0,0x7C,0x80,0x84,0x70,0x74,0x78,0x64,0x68,0x6C,0x88,0x8C,0x90,0xCC,0xD0,0xD4,0xA4,0xA8,0xAC,0xC0,0xC4,0xC8]
creation=(r/'src/integration/lagi_battle_models.inc').read_text().split('bool lagiInitBattleEnemyModelTargets')[0]
core=(r/'extern/Azel/AzelLib/battle/BTL_A3/baldor.cpp').read_text()
core=core[core.index('void initBattleTargetables('):core.index('void Baldor_initSub0(')]
with tempfile.TemporaryDirectory(prefix='lagi-lathum-rig-') as tmp:
 d=Path(tmp)
 for name,text in [('rig.inc',(r/'src/integration/lagi_lathum_rig.inc').read_text()),('model_creation.inc',creation),('core_targets.inc',core)]: (d/name).write_text(text)
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(d),str(r/'tests/lathum_rig_test.cpp'),'/Fe:'+str(d/'test.exe'),'/Fo:'+str(d/'test.obj')],check=True)
 subprocess.run([str(d/'test.exe'),str(p),str(p.with_name('ZUSU.MCB')),str(p.with_name('COMMON.DAT'))],check=True)
 flatten=(r/'extern/Azel/AzelLib/battle/BTL_A3/urchin.cpp').read_text()
 flatten=flatten[flatten.index('void urchinUpdateSub3('):flatten.index('void urchinUpdateSub4(')]
 (d/'flatten.inc').write_text(flatten)
 (d/'draw.inc').write_text((r/'src/integration/lagi_lathum_draw.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(d),str(r/'tests/lathum_draw_test.cpp'),'/Fe:'+str(d/'draw.exe'),'/Fo:'+str(d/'draw.obj')],check=True)
 subprocess.run([str(d/'draw.exe'),str(p),str(p.with_name('ZUSU.MCB')),str(p.with_name('COMMON.DAT'))],check=True)

 (d/'damage.inc').write_text((r/'src/integration/lagi_lathum_damage.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(d),str(r/'tests/lathum_damage_test.cpp'),'/Fe:'+str(d/'damage.exe'),'/Fo:'+str(d/'damage.obj')],check=True)
 subprocess.run([str(d/'damage.exe'),str(p),str(p.with_name('ZUSU.MCB')),str(p.with_name('COMMON.DAT'))],check=True)

print('PASS original descriptors/target groups/init operands and generated integration; BTL_A5_3 SHA256',hashlib.sha256(b).hexdigest())
