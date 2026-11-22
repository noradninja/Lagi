from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='lagi-variant-damage-') as d:
 p=Path(d);(p/'variant.inc').write_text((r/'src/integration/lagi_garil_variant_damage.inc').read_text());(p/'shared.inc').write_text((r/'src/integration/lagi_battle_model_damage.inc').read_text())
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(p),'/I'+str(r/'include'),str(r/'tests/garil_variant_damage_test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
assert 'const bool damaged=lagiUpdateGarilVariantDamage(pThis);' in s
assert 'pThis->mB2_health -= t.m58;' not in s
assert s.index('lagiUpdateGarilVariantMembership(pThis);')<s.index('lagiUpdateGarilVariantDamageValues(pThis);')<s.index('const bool damaged=lagiUpdateGarilVariantDamage(pThis);')
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value in [(0x60555D2,0xE52D),(0x60555E0,0xE405),(0x6055644,0x144),(0x6055646,0x14A),(0x60551F8,0x6120)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Native variant damage contract changed at {ea:08X}'
print('PASS original damage-value literals and generated shared-pipeline integration')
