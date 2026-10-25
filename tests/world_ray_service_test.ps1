param([string]$ScratchRoot='../world-ray-service-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=(Get-Content -Raw "$repo/src/integration/lagi_world_ray_vita.cpp").Replace("`r`n","`n")
$source=$source.Replace('#include "lagi/lagi_azel_upstream_prelude.h"','').Replace('#include "kernel/rayDisplay.h"','')
[IO.File]::WriteAllText("$scratch/world_service.inc",$source)
$mock=@'
static s32 lagiWorldRayProjection(std::array<sVec3_FP,2>& view,s32 width,s_graphicEngineStatus_405C& clip,sScreenQuad3& screen) {
 ++projections;assert(width==42);
 if(!lagiWorldRayClipDepth(view,clip))return 0;
 projectedView=view;screen=nextScreen;return 1;
}
'@
[IO.File]::WriteAllText("$scratch/lagi_world_ray_projection.inc",$mock)
$native=(Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/rayDisplay.cpp").Replace("`r`n","`n")
$start=$native.IndexOf('s32 rayComputeDisplayMatrix_fixedWidth(')
$end=$native.IndexOf('s32 rayComputeDisplayMatrix_2Width(',$start)
if($start-lt 0 -or $end-le $start){throw 'Native world projection boundaries changed'}
$expected=$native.Substring($start,$end-$start).Replace('s32 rayComputeDisplayMatrix_fixedWidth(','static s32 lagiWorldRayProjection(').Replace('isGunShotVisible(param_1, param_3)','lagiWorldRayClipDepth(param_1, param_3)')
if((Get-Content -Raw "$repo/$BattleBuild/lagi_world_ray_projection.inc").Replace("`r`n","`n") -cne $expected){throw 'Generated world projection differs from native body beyond explicit name/clip boundary'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/world_ray_service_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'World ray service compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'World ray service assertions failed'}
