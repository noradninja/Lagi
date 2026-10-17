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
if($normalNinja.Contains('lagi_battle_rays_vita.cpp.obj')){throw 'Battle-only ray service leaked into ordinary build'}
if($normalNinja.Contains('lagi_battle_graphics_vita.cpp.obj')){throw 'Battle-only graphics service leaked into ordinary build'}
if(!$battleNinja.Contains('lagi_battle_graphics_vita.cpp.obj')){throw 'Battle graphics service missing'}
$encounter=Get-Content -Raw "$battle/azel_upstream/field_fieldEncounterTask.cpp"
if(($encounter.Split('lagiRestoreBattleGraphics(pThis->m20);').Count-1) -ne 2){throw 'Both battle return restoration sites must be wired'}
if(!$encounter.Contains('lagiDiscardBattleGraphics(pThis->m20);')){throw 'Encounter teardown leaks graphics backup'}
if(!$encounter.Contains('if (!pThis->m20)')){throw 'Encounter must not proceed after capture failure'}
$normalEncounter=Get-Content -Raw "$normal/azel_upstream/field_fieldEncounterTask.cpp"
if($normalEncounter.Contains('lagiCaptureBattleGraphics')){throw 'Ordinary encounter graphics path unexpectedly changed'}
$objects=@('battle_battleEngine','battle_battleEngineSub0','battle_battleEngineSub1',
 'battle_battleMainTask','battle_battleGenericData','battle_battleDragon','battle_battleGrid',
 'battle_BTL_A3_BTL_A3','battle_BTL_A3_BTL_A3_data')
foreach($object in $objects) {
 $name="$object.cpp.obj"
 if(!$battleNinja.Contains($name)){throw "Battle source missing: $name"}
 if($normalNinja.Contains($name)){throw "Battle source leaked into normal configuration: $name"}
 if(!(Test-Path "$battle/CMakeFiles/lagi.dir/azel_upstream/$name")){throw "Battle object not compiled: $name"}
}
$nativeRoot=Join-Path $repo 'extern/Azel/AzelLib'
$expanded=@(Get-ChildItem "$nativeRoot/battle/*.cpp";Get-ChildItem "$nativeRoot/battle/BTL_A3/*.cpp")
foreach($file in $expanded) {
 $relative=$file.FullName.Substring($nativeRoot.Length+1).Replace('\','/');
 $name=$relative.Replace('/','_')+'.obj'
 if(!$battleNinja.Contains($name)){throw "Expanded native battle source missing: $relative"}
 if(!(Test-Path "$battle/CMakeFiles/lagi.dir/azel_upstream/$name")){throw "Expanded battle source not compiled: $relative"}
}
foreach($name in @('battle_interpolators_FPInterpolator.cpp.obj','battle_interpolators_vec2FPInterpolator.cpp.obj','kernel_monsterPart.cpp.obj')) {
 if(!(Test-Path "$battle/CMakeFiles/lagi.dir/azel_upstream/$name")){throw "Native dependency not compiled: $name"}
 if($normalNinja.Contains($name)){throw "Native battle dependency leaked into ordinary build: $name"}
}
foreach($folder in @($battle,$normal)) {
 $module=Get-Content -Raw "$folder/azel_upstream/kernel_moduleManager.cpp"
 if($module -notmatch 'NULL,\s*// 4: battle - deferred'){throw 'Incomplete battle dispatch unexpectedly enabled'}
 $overlay=Get-Content -Raw "$folder/azel_upstream/battle_battleOverlay.cpp"
 if($overlay -match 'overlayStart_BTL_\w+\(pThis\)'){throw 'Incomplete overlay startup unexpectedly reachable'}
}
Write-Output "PASS: $($expanded.Count) shared/A3 objects plus native interpolation/monster dependencies compiled; ordinary build and dispatch/overlay safeguards retained. Not a retained-root link or hardware test."
