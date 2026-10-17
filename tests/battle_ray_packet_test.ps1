param([string]$ScratchRoot='../battle-ray-packet-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$native=(Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/rayDisplay.cpp").Replace("`r`n","`n")
$begin=$native.IndexOf('void sendRaySegmentToVdp1(');$end=$native.IndexOf('static void computeRayVertexColor',$begin)
[IO.File]::WriteAllText("$scratch/native.inc",$native.Substring($begin,$end-$begin).Replace('sendRaySegmentToVdp1(','referenceSend('))
$begin=$native.IndexOf('s32 rayComputeDisplayMatrix_fixedWidth(');$end=$native.IndexOf('s32 rayComputeDisplayMatrix_2Width(',$begin)
if((Get-Content -Raw "$repo/$BattleBuild/lagi_battle_ray_projection.inc").Replace("`r`n","`n") -cne $native.Substring($begin,$end-$begin)){throw 'Battle projection is not the exact native body'}
$service=Get-Content -Raw "$repo/src/integration/lagi_battle_rays_vita.cpp"
$begin=$service.IndexOf('void sendRaySegmentToVdp1(');$end=$service.IndexOf('void displayRaySegment(',$begin)
[IO.File]::WriteAllText("$scratch/service.inc",$service.Substring($begin,$end-$begin))
[IO.File]::WriteAllText("$scratch/wrapper.inc",$service.Substring($end))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_ray_packet_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Battle ray packet test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Battle ray packet differential failed'}
