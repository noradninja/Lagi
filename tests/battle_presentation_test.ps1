param([string]$ScratchRoot='../battle-presentation-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_scene_bridge.cpp"
$begin=$source.IndexOf('void sync_battle_presentation_state()');$end=$source.IndexOf('void sync_field_presentation_state()',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Battle presentation production boundary changed'}
[IO.File]::WriteAllText("$scratch/service.inc",$source.Substring($begin,$end-$begin))
$grid=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_battleGrid.cpp"
if($grid -notmatch 'generateCameraMatrix\(&cameraProperties2, transformedCameraPosition, transformedCameraTarget, transformedCameraUp\);\s*lagi::azel_bridge::capture_native_scene_view_matrix\(\);'){throw 'Battle root must be captured immediately after native camera generation'}
foreach($file in @('src/integration/lagi_runtime.cpp','src/integration/lagi_scene_bridge.cpp','src/platform/vita/neptune_renderer.cpp')){
 if(!(Get-Content -Raw "$repo/$file").Contains('lagi::native_scene_mode(')){throw "Native scene capability boundary absent: $file"}
}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if($renderer.Contains('g_sceneGameMode == 3u ? depthFunc') -or $renderer.Contains('g_sceneGameMode == 3u ? SCE_GXM_DEPTH_WRITE_ENABLED')){throw 'Direct battle mesh depth still uses the unconditional overlay policy'}
if($renderer -notmatch 'const bool worldDepth = command.hasWorldDepth && !command.fieldRadar &&\s*lagi::native_scene_depth_mode\(g_sceneGameMode\)'){throw 'Battle projected world sprites must keep their supplied endpoint depth'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/battle_presentation_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Battle presentation test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Battle presentation test failed'}
