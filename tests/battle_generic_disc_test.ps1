param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$ScratchRoot='../battle-generic-disc-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
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
[IO.File]::WriteAllText("$scratch/dispatch.inc","constexpr u32 formationFunctions[]={$formations};`nconstexpr u32 mapFunctions[]={$maps};`n")
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
