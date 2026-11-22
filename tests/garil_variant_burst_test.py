from pathlib import Path
import subprocess,sys,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
assert 'lagiFireGarilVariantBurst(pThis);' in s
with tempfile.TemporaryDirectory(prefix='lagi-variant-burst-') as d:
 p=Path(d);(p/'burst.inc').write_text((r/'src/integration/lagi_garil_variant_burst.inc').read_text())
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(p),'/I'+str(r/'include'),str(r/'tests/garil_variant_burst_test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
anchors={0x60AD104:(0,4),0x60AD108:(0x60544DC,4),0x60AD10C:(0x60545A4,4),0x60AD110:(0,4),0x60546E8:(0x28000,4),0x60546E6:(0xFF50,2),0x60546DC:(0x1FFF,2),0x60546DE:(0xF001,2),0x60546E0:(0x7FF,2),0x60546E2:(0xFC01,2),0x6054452:(0xE01B,2),0x605445A:(0x7065,2),0x605448C:(0xC90E,2),0x605448E:(0x700D,2),0x605453E:(0xE52C,2),0x605454C:(0xE702,2),0x60545D8:(0x800,2)}
for ea,(value,n) in anchors.items():
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Native burst contract changed at {ea:08X}'
print('PASS original Saturn callback, motion, RNG, timer and damage literals')
