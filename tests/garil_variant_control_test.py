from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
u=s[s.index('static void BTL_A5_formationVariant_Update'):]
assert u.index('lagiUpdateGarilVariantQuadrants(pThis);')<u.index('formationUpdatePosition(')<u.index('    switch (pThis->m28_state)')
assert 'pThis->m28_state = 8;' in u and 'pThis->m28_state = 0xB;' not in u
intro=u[u.index('    case 2:'):u.index('    case 3:')]
assert 'pThis->m28_state=5;' in intro
phases=u[u.index('    case 3:'):u.index('\n    }\n}\n\n// 060547da')]
assert phases.strip()==(r/'src/integration/lagi_garil_variant_phases.inc').read_text().strip()
with tempfile.TemporaryDirectory(prefix='lagi-variant-control-') as d:
 p=Path(d);(p/'membership.inc').write_text((r/'src/integration/lagi_garil_variant_membership.inc').read_text());(p/'quadrants.inc').write_text((r/'src/integration/lagi_garil_variant_quadrants.inc').read_text());(p/'control.inc').write_text((r/'src/integration/lagi_garil_variant_control.inc').read_text());(p/'phases.inc').write_text(phases)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(p),'/I'+str(r/'src/integration'),str(r/'tests/garil_variant_control_test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
anchors={0x6054C9C:(0x04000000,4),0x6054C8E:(0xF3,2),0x6054DAC:(0xF8,2),0x6054B7E:(0xF2,2),0x6054C8C:(0xE0,2),0x6054B68:(0xE40F,2),0x6054B64:(0xE51E,2),0x6054BA2:(0xE369,2),0x6054AF2:(0xE30F,2),0x6054BCE:(0xE34B,2),0x6054C9A:(0x40D2,2),0x60A2BE4:(0xFBFFFFFF,4),0x60A2BE8:(0x02000000,4)}
for ea,(value,n) in anchors.items():
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Native variant controller contract changed at {ea:08X}'
print('PASS native Saturn selector flags, field mapping, release protocol, names and timer literals')
for i in range(4):
 ea=0x60AD194+i*4
 assert int.from_bytes(data[ea-base:ea-base+4],'big')==i
print('PASS 6400 quadrant classifications, native shift table and generated interpolation order')
assert s.index('lagiUpdateGarilVariantMembership(pThis);')<s.index('    // damage check')
assert 'pParent->mF4_entityPositions[entityIndex]=dir;' in s
assert int.from_bytes(data[0x6054E7A-base:0x6054E7C-base],'big')==0x2210
print('PASS 786432 membership cases, native initial quadrant store and generated actor update order')
assert s.index('lagiConsumeGarilVariantDamage(pThis);')<s.index('    // state machine')
assert s.count('lagiSetGarilVariantReaction(pThis,true);')==1
assert s.count('lagiSetGarilVariantReaction(pThis,false);')==2
for ea,value in [(0x6055100,0xDFFFFFFF),(0x6055104,0xBFFFFFFF)]:
 assert int.from_bytes(data[ea-base:ea-base+4],'big')==value
print('PASS reaction ownership, all damage-byte values and native flag-clear literals')
