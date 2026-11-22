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
assert 'lagiUpdateLathumEmergence(' in generated
assert 'lagiUpdateLathumReaction(' in generated
assert 'lagiSelectLathumTurn(' in generated
assert 'lagiCreateLathumRing(' in generated and 'lagiUpdateLathumAttackTwo(' in generated
assert 'lagiUpdateLathumAttackThree(' in generated
assert 'lagiUpdateLathumRig(pThis,pThis->rig,*overlay->m4_battleEngine,*overlay->m8_gridTask,*overlay->m18_dragon)' in generated
assert 'static void Update(sBTL_A5_3_LathumFormation* pThis) { Unimplemented(); }' not in generated
for ea,value,n in [(0x06055A50,0x926A,2),(0x06055B28,0x2000,2),
 (0x06055B44,0x64000,4),(0x06055AB2,0x8821,2),
 (0x06055B48,0x06021798,4),(0x06055B4C,0x06021882,4),
 (0x06055B74,0x012C,2),(0x06055B86,0x7B14,2),
 (0x06055C74,0x0605B1DC,4),(0x06055C6A,240,2),
 (0x06055C0A,0xE30F,2),(0x06055C20,0x4215,2),
 (0x06055FF0,0x3B2,2),(0x060560C4,0x30000,4),
 (0x060561BC,0x80000,4),(0x0605607A,0xE369,2),
 (0x0605611E,0xE369,2),(0x060561E6,0x8801,2)]:
 assert u(ea,n)==value,f'Original death/main Update differs at {ea:08X}'
assert [u(0x060A3FB0+i*4,4) for i in range(3)]==[0,0x1000,0]
assert [u(0x060A3FBC+i*4,4) for i in range(3)]==[0,0x1000,0x1000]

for ea,value,n in [(0x06055744,0x5B05B,4),(0x0605574C,0xFA000,4),
 (0x0605575C,0x96000,4),(0x06055874,0x32000,4),
 (0x0605584A,0xE515,2),(0x060558EA,0xE210,2),
 (0x060559EC,0x060A3F70,4),(0x060559F0,0x060A3B9C,4),
 (0x060559AA,0x4211,2),(0x06055A06,0xE009,2),
 (0x0606FC5A,0x890C,2),(0x0606FCF4,0x060361F4,4),
 (0x0606FC62,0xEC1F,2)]:
 assert u(ea,n)==value,f'Original repair/shared particle phase differs at {ea:08X}'
assert [u(0x060A3F70+i*4,4) for i in range(3)]==[0,0xFFFFFFA8,0]

for ea,value,n in [(0x06056F68,0x06056A60,4),(0x06056F64,0x120,2),
 (0x06056F80,0x06035664,4),(0x06056F84,0x1E0000,4),
 (0x06056F66,0x8001,2),(0x06056F32,0xC92F,2),
 (0x06056BEA,0xCCC,2),(0x06056BF8,0x71C71C,4),
 (0x06056C04,0x20216E80,4),(0x06056C08,0x20217E80,4),
 (0x06056C18,0x4000000,4),(0x06056E00,0x10000000,4),
 (0x06056F62,0x1C0,2),(0x06056E0E,0x03E6,2),
 (0x06054AA0,0x06056E2E,4),(0x06054BBE,0x2C0,2),
 (0x06054BD0,0x06035686,4),(0x06054D22,0xFEA0,2),
 (0x06054CAA,0xE55C,2),(0x06054D2C,0x0605C050,4)]:
 assert u(ea,n)==value,f'Original ring/attack2 differs at {ea:08X}'

assert 'lagiCreateLathumProjectile(' in generated and 'lagiUpdateLathumAttackOne(' in generated
for ea,value,n in [(0x060A401C,0x06056610,4),(0x060A4020,0x060567F0,4),
 (0x060569B6,0xBC,2),(0x060569B8,0xB8,2),(0x06056866,0xC91F,2),
 (0x0605686E,0x704B,2),(0x06056872,0x7658,2),
 (0x060569DC,0x06036038,4),(0x060569E0,0xA000,4),
 (0x060569E8,0x06035664,4),(0x060569EC,0x06035644,4),
 (0x060569F4,0x060A3AD8,4),(0x06056A42,0xE305,2),
 (0x060567B0,0xFFF,2),(0x060567B2,0x28F,2),
 (0x0605679A,0xE51C,2),(0x060567AA,0xE703,2),
 (0x060567C8,0x0605C050,4),(0x0605683C,0x30000,4),
 (0x0605477C,0x06056840,4),(0x0605476A,0xE35A,2),
 (0x0605478A,0x4211,2),(0x06054616,0x2000,2)]:
 assert u(ea,n)==value,f'Original timed projectile/attack1 differs at {ea:08X}'

for ea,value,n in [(0x060544E6,0x3CC,2),(0x060544F0,0x060598D0,4),
 (0x060544F8,0x060687D6,4),(0x060544FC,0x06050224,4),
 (0x0605445C,0x8503,2),(0x0605445E,0xC804,2),
 (0x06054468,0xE403,2),(0x0605446E,0xE40B,2),
 (0x060544B0,0xE402,2),(0x060544CA,0xE401,2)]:
 assert u(ea,n)==value,f'Original turn selection differs at {ea:08X}'
# Source-mapped A3 item-menu read independently anchors primary input port;
# native port2 follows one complete6C-byte input-device record later.
a3=p.with_name('BTL_A3.PRG').read_bytes()
assert int.from_bytes(a3[0x06074318-base:0x06074318-base+4],'big')==0x060501B8
assert 0x060501B8+0x6C==u(0x060544FC,4)

for ea,value,n in [(0x06055614,0x060598F2,4),(0x06055618,0x06059724,4),
 (0x06055556,0xE402,2),(0x06055576,0x7658,2),
 (0x0605560A,0x2000,2),(0x060555B6,0xE50A,2),
 (0x06055634,0x060A3F64,4),(0x06055638,0x06068B66,4),
 (0x06055610,0x100,2)]:
 assert u(ea,n)==value,f'Original reaction contract differs at {ea:08X}'

assert [u(0x060A3F04+i*12,2) for i in range(3)]==[4,5,0xFFFF]
assert [u(0x060A3F28+i*12,2) for i in range(5)]==[6,7,8,9,0xFFFF]
for ea,value,n in [(0x06054FF8,0x0607950C,4),(0x06054FFC,0x060794AA,4),
 (0x06055000,0x06059724,4),(0x06055004,0x06079416,4),
 (0x06055008,0x0607974A,4),(0x0605500C,0x06079778,4),
 (0x06055018,0xFFF55ED2,4),(0x0605501C,0xFFF94000,4),
 (0x060550D8,0xFFFAE000,4),(0x060550DC,0x7C000,4),
 (0x060550E0,0x06079A00,4),(0x060551AA,0xE264,2),
 (0x060552B0,0x50000,4),(0x06055410,0x060A3C60,4),(0x06055418,0x0606FBD0,4),
 (0x0605541C,0x060A3F04,4),(0x06055420,0x06068B62,4),
 (0x06055438,0x060A3F28,4),(0x06055538,0x100,2)]:
 assert u(ea,n)==value,f'Original emergence contract differs at {ea:08X}'

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

 particle_source=(r/'extern/Azel/AzelLib/battle/particleEffect.cpp').read_text()
 phase_start=particle_source.index('    if (lifetime) {')
 phase_end=particle_source.index('    if (gouraudColors == nullptr)',phase_start)
 (d/'particle_phase.inc').write_text('static void lagiTestParticlePhase(sParticleTask* pNewParticleEffect, int lifetime) {\n'+particle_source[phase_start:phase_end]+'}\n')
 (d/'damage_word.inc').write_text((r/'src/integration/lagi_lathum_damage.inc').read_text().split('static void lagiUpdateLathumDamage')[0])
 (d/'emergence.inc').write_text((r/'src/integration/lagi_lathum_emergence.inc').read_text())
 (d/'reaction.inc').write_text((r/'src/integration/lagi_lathum_reaction.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/I'+str(d),str(r/'tests/lathum_emergence_test.cpp'),'/Fe:'+str(d/'emergence.exe'),'/Fo:'+str(d/'emergence.obj')],check=True)
 subprocess.run([str(d/'emergence.exe'),str(p),str(p.with_name('ZUSU.MCB')),str(p.with_name('COMMON.DAT'))],check=True)

 (d/'turn.inc').write_text((r/'src/integration/lagi_lathum_turn.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/O2','/EHsc','/I'+str(d),str(r/'tests/lathum_turn_test.cpp'),'/Fe:'+str(d/'turn.exe'),'/Fo:'+str(d/'turn.obj')],check=True)
 subprocess.run([str(d/'turn.exe'),str(p)],check=True)

 (d/'projectile.inc').write_text((r/'src/integration/lagi_lathum_projectile.inc').read_text())
 (d/'ring.inc').write_text((r/'src/integration/lagi_lathum_ring.inc').read_text())
 (d/'attack_one.inc').write_text((r/'src/integration/lagi_lathum_attack_one.inc').read_text())
 (d/'attack_two.inc').write_text((r/'src/integration/lagi_lathum_attack_two.inc').read_text())
 (d/'attack_three.inc').write_text((r/'src/integration/lagi_lathum_attack_three.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/O2','/EHsc','/I'+str(d),str(r/'tests/lathum_projectile_test.cpp'),'/Fe:'+str(d/'projectile.exe'),'/Fo:'+str(d/'projectile.obj')],check=True)
 subprocess.run([str(d/'projectile.exe'),str(p),str(p.with_name('COMMON.DAT'))],check=True)

 (d/'damage.inc').write_text((r/'src/integration/lagi_lathum_damage.inc').read_text())
 (d/'death.inc').write_text((r/'src/integration/lagi_lathum_death.inc').read_text())
 (d/'update.inc').write_text((r/'src/integration/lagi_lathum_update.inc').read_text())
 subprocess.run(['cl','/nologo','/std:c++20','/O2','/EHsc','/I'+str(d),str(r/'tests/lathum_update_test.cpp'),'/Fe:'+str(d/'update.exe'),'/Fo:'+str(d/'update.obj')],check=True)
 subprocess.run([str(d/'update.exe'),str(p),str(p.with_name('ZUSU.MCB')),str(p.with_name('COMMON.DAT'))],check=True)

print('PASS original descriptors/target groups/init operands and generated integration; BTL_A5_3 SHA256',hashlib.sha256(b).hexdigest())
