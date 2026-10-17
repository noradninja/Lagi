param([string]$BattleBuild='build-battle-dev',[string]$NormalBuild='build-menu-verify')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$battle=Join-Path $repo $BattleBuild
$normal=Join-Path $repo $NormalBuild
$battleCache=Get-Content -Raw "$battle/CMakeCache.txt"
$normalCache=Get-Content -Raw "$normal/CMakeCache.txt"
if($battleCache -notmatch 'LAGI_BATTLE_NATIVE_BUILD:BOOL=ON'){throw 'Battle configuration must explicitly opt in'}
if($normalCache -notmatch 'LAGI_BATTLE_NATIVE_BUILD:BOOL=OFF'){throw 'Normal configuration must retain encounters-disabled default'}
$battleNinja=Get-Content -Raw "$battle/build.ninja"
$normalNinja=Get-Content -Raw "$normal/build.ninja"
$objects=@('battle_battleEngine','battle_battleEngineSub0','battle_battleEngineSub1',
 'battle_battleMainTask','battle_battleGenericData','battle_battleDragon','battle_battleGrid',
 'battle_BTL_A3_BTL_A3','battle_BTL_A3_BTL_A3_data')
foreach($object in $objects) {
 $name="$object.cpp.obj"
 if(!$battleNinja.Contains($name)){throw "Battle source missing: $name"}
 if($normalNinja.Contains($name)){throw "Battle source leaked into normal configuration: $name"}
 if(!(Test-Path "$battle/CMakeFiles/lagi.dir/azel_upstream/$name")){throw "Battle object not compiled: $name"}
}
foreach($folder in @($battle,$normal)) {
 $module=Get-Content -Raw "$folder/azel_upstream/kernel_moduleManager.cpp"
 if($module -notmatch 'NULL,\s*// 4: battle - deferred'){throw 'Incomplete battle dispatch unexpectedly enabled'}
 $overlay=Get-Content -Raw "$folder/azel_upstream/battle_battleOverlay.cpp"
 if($overlay -match 'overlayStart_BTL_\w+\(pThis\)'){throw 'Incomplete overlay startup unexpectedly reachable'}
}
Write-Output 'PASS: nine real native battle objects compiled only when opted in; ordinary build and dispatch/overlay safeguards retained. Not a reachable-combat or hardware test.'
