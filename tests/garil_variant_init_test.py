from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
fixture=(r/'tests/garil_variant_init_test.cpp').read_text().replace('// PRODUCTION_INIT',(r/'src/integration/lagi_garil_variant_init.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-variant-init-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
assert 'if(!lagiInitGarilVariantModels(pEntity,pBundle))return nullptr;' in s
assert 's16 enemyTypeId = pParent->mE0_entityGroup.mC_formationNameIndex;' in s
assert s.count('std::span<sBattleTargetable>(pThis->m2C_models[0].m54_targetables,pThis->m2C_models[0].m52_targetableCount)')==1
assert 'for (auto& t : pThis->mC0_targetables)' not in s
assert 'getAnimation(modelOffset)' not in s
data=Path(sys.argv[1]).read_bytes();base=0x6054000
assert data[0x60AD5C0-base:0x60AD5C8-base]==bytes.fromhex('00040040060ad278')
for ea,value in [(0x6054E8A,0x7548),(0x6054F0E,0x7244),(0x6054F12,0xE208),(0x6054F46,0xE405),(0x6054F48,0xE22D),(0x6054E42,0x8576)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Original variant initialization changed at {ea:08X}'
print('PASS original descriptor, animation, collision, enemy ID and live target-array integration')
assert 'getAnimation(0x4C)' in s and 'getAnimation(0x50)' in s and 'getAnimation(0x48)' in s
for ea,value in [(0x605513E,0x754C),(0x605526A,0x7550),(0x6055316,0x7548)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value
print('PASS original idle/reaction/attack animation slots')
