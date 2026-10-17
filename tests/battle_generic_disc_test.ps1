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
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "/I$repo/extern/Azel/AzelLib" "$PSScriptRoot/battle_generic_disc_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Disc-backed generic-data test compile failed'}
& "$scratch/test.exe" $DiscImage
if($LASTEXITCODE -ne 0){throw 'Disc-backed generic-data test failed'}
