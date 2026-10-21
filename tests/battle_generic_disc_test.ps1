param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$ScratchRoot='../battle-generic-disc-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$a3=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_BTL_A3_BTL_A3_data.cpp"
if(!$a3.Contains('j<animationCount') -or !$a3.Contains('ptrTo1C = animationTable + j * 0x24;')){throw 'Generated A3 animation reader missing count/stride adapter'}
$begin=$a3.IndexOf('sGenericFormationData* readUrchinFormation(')
$end=$a3.IndexOf('p_workArea BTL_A3_data::invokeCreateEffect(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native A3 formation reader boundary changed'}
[IO.File]::WriteAllText("$scratch/formation.inc",$a3.Substring($begin,$end-$begin))
$header=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/BTL_A3_UrchinFormation_native.h"
$end=$header.IndexOf('struct BTL_A3_UrchinFormation :')
if($end -lt 0){throw 'Native A3 formation header boundary changed'}
[IO.File]::WriteAllText("$scratch/formation_types.inc",$header.Substring(0,$end))
$digits=Get-Content -Raw "$repo/src/integration/lagi_battle_damage_digits_vita.cpp"
$begin=$digits.IndexOf('void lagiDrawBattleDamageDigits(')
if($begin -lt 0){throw 'Damage digit service boundary changed'}
[IO.File]::WriteAllText("$scratch/digits.inc",$digits.Substring($begin))
$charge=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_battlePowerGauge.cpp"
$begin=$charge.IndexOf('static void sGaugeIncreaseEffect_Draw(')
$end=$charge.IndexOf('static const sGaugeIncreaseEffect::TypedTaskDefinition',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native charge-effect draw boundary changed'}
[IO.File]::WriteAllText("$scratch/charge.inc",$charge.Substring($begin,$end-$begin))
$enemy=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_BTL_A3_urchin.cpp"
foreach($required in @('m2C_deathMode == 4','u8 deathMode = pThis->mCC->m2C_deathMode;','m2D_hitSound != 0xFF','m2E_deathSound != 0xFF','playSystemSoundEffect(pThis->mCC->m2D_hitSound);','playSystemSoundEffect(pThis->mCC->m2E_deathSound);')){
 if(!$enemy.Contains($required)){throw "Missing generated enemy parameter consumer: $required"}
}
if($enemy.Contains('m28[4]') -or $enemy.Contains('m28[1] != -1') -or $enemy.Contains('m28[2] != -1')){throw 'Enemy death/sound still reads quadrant colors'}
$generic=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_battleGenericData.cpp"
$begin=$generic.IndexOf('const std::vector<quadColor> BTL_GenericData::m60AE424')
if($begin -lt 0){throw 'Native generic-data boundary changed'}
[IO.File]::WriteAllText("$scratch/generic.inc",$generic.Substring($begin))
$reader=Get-Content -Raw "$repo/src/integration/lagi_vdp1_animated_quad_vita.cpp"
$begin=$reader.IndexOf('std::vector<sVdp1Quad> initVdp1Quad(');$end=$reader.IndexOf('void particleInitSub(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Shared native quad reader boundary changed'}
[IO.File]::WriteAllText("$scratch/quad.inc",$reader.Substring($begin,$end-$begin))
$dispatch=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/BTL_A3/BTL_A3_data.cpp"
$begin=$dispatch.IndexOf('void BTL_A3_data::invoke(sSaturnPtr Func, s_workAreaCopy* pParent, u32 arg0, u32 arg1)')
$middle=$dispatch.IndexOf('void BTL_A3_data::invoke(sSaturnPtr Func, s_workAreaCopy* pParent)',$begin)
$end=$dispatch.IndexOf('sGenericFormationData* readUrchinFormation(',$middle)
if($begin -lt 0 -or $middle -le $begin -or $end -le $middle){throw 'Native A3 dispatch boundary changed'}
$formations=([regex]::Matches($dispatch.Substring($begin,$middle-$begin),'case (0x[0-9a-fA-F]+):') | ForEach-Object {$_.Groups[1].Value}) -join ','
$maps=([regex]::Matches($dispatch.Substring($middle,$end-$middle),'case (0x[0-9a-fA-F]+):') | ForEach-Object {$_.Groups[1].Value}) -join ','
$effectBegin=$a3.IndexOf('p_workArea BTL_A3_data::invokeCreateEffect(')
$effectEnd=$a3.IndexOf('BTL_A3_data::BTL_A3_data()', $effectBegin)
if($effectBegin -lt 0 -or $effectEnd -le $effectBegin){throw 'Live A3 effect dispatch boundary changed'}
$effects=([regex]::Matches($a3.Substring($effectBegin,$effectEnd-$effectBegin),'case (0x[0-9a-fA-F]+):') | ForEach-Object {$_.Groups[1].Value}) -join ','
[IO.File]::WriteAllText("$scratch/dispatch.inc","constexpr u32 formationFunctions[]={$formations};`nconstexpr u32 mapFunctions[]={$maps};`nconstexpr u32 effectFunctions[]={$effects};`n")
$overlay=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/battleOverlay.h"
$begin=$overlay.IndexOf('struct sOverlayBattleEntry');$end=$overlay.IndexOf('struct battleOverlay :',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native battle entry reader boundary changed'}
[IO.File]::WriteAllText("$scratch/entry.inc",$overlay.Substring($begin,$end-$begin))
$manager=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/battleManager.cpp"
$begin=$manager.IndexOf('void remapBattleOverlayId(');$end=$manager.IndexOf('static int loadBattleOverlay_debug(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native battle remap boundary changed'}
[IO.File]::WriteAllText("$scratch/remap.inc",$manager.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "/I$repo/extern/Azel/AzelLib" "$PSScriptRoot/battle_generic_disc_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Disc-backed generic-data test compile failed'}
& "$scratch/test.exe" $DiscImage
if($LASTEXITCODE -ne 0){throw 'Disc-backed generic-data test failed'}
