param([string]$ScratchRoot='../battle-quad-reader-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$native=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/battleGenericData.cpp"
$begin=$native.IndexOf('std::vector<sVdp1Quad> initVdp1Quad(')
$end=$native.IndexOf('// Parse a sLaserData',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native reader boundary changed'}
[IO.File]::WriteAllText("$scratch/native.inc",$native.Substring($begin,$end-$begin).Replace('initVdp1Quad(','referenceQuadReader('))
$service=Get-Content -Raw "$repo/src/integration/lagi_vdp1_animated_quad_vita.cpp"
$begin=$service.IndexOf('std::vector<sVdp1Quad> initVdp1Quad(')
$end=$service.IndexOf('void particleInitSub(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Service reader boundary changed'}
[IO.File]::WriteAllText("$scratch/service.inc",$service.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_quad_reader_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Quad reader test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Quad reader differential failed'}
