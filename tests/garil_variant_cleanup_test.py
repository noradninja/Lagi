from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
retire=(r/'src/integration/lagi_battle_models.inc').read_text();retire=retire[retire.index('void lagiRetireBattleEnemyModelTargets'):]
reaction=(r/'src/integration/lagi_garil_variant_membership.inc').read_text();reaction=reaction[reaction.index('static void lagiSetGarilVariantReaction'):]
fixture=(r/'tests/garil_variant_cleanup_test.cpp').read_text().replace('// RETIRE',retire).replace('// REACTION',reaction).replace('// CLEANUP',(r/'src/integration/lagi_garil_variant_cleanup.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-variant-cleanup-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
assert '&lagiDeleteGarilVariantActor,' in s
assert 'register_work_area_disposer(pEntity,[](void* area){delete static_cast<sBTL_A5_VariantEnemy*>(area);});' in s
assert 'lagiBattleEnemyDefeated(pThis->mAC_lifeMeter);' in s
print('PASS generated delete callback, concrete disposer and shared reward notification')
