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
if(!$battleNinja.Contains('town_e011_twn_e011.cpp.obj') -or $normalNinja.Contains('town_e011_twn_e011.cpp.obj')){throw 'E011 cinematic must remain native-battle opt-in'}
$battleTown=Get-Content -Raw "$battle/azel_upstream/town_town.cpp"
$normalTown=Get-Content -Raw "$normal/azel_upstream/town_town.cpp"
if(!$battleTown.Contains('gFieldOverlayFunction = overlayStart_TWN_E011;') -or $normalTown.Contains('gFieldOverlayFunction = overlayStart_TWN_E011;')){throw 'E011 dispatch does not match compiled cinematic capability'}
$cinematic=Get-Content -Raw "$battle/azel_upstream/town_e011_twn_e011.cpp"
foreach($nativeStep in @('sTownOverlay("TWN_E011.PRG")','registerNpcs(gTWN_E011->mTownSetups','startScriptTask(pThis);','startE011BackgroundTask(pThis);','startCameraTask(pThis);')){
 if(!$cinematic.Contains($nativeStep)){throw "E011 native startup missing: $nativeStep"}
}
if(!$battleNinja.Contains('LAGI_NATIVE_TASK_HEAP_CLEANUP=1') -or $normalNinja.Contains('LAGI_NATIVE_TASK_HEAP_CLEANUP=1')){throw 'Task heap disposal must remain native-battle opt-in'}
if(!$battleNinja.Contains('LAGI_BATTLE_BUILD_NATIVE=1') -or !$normalNinja.Contains('LAGI_BATTLE_BUILD_NATIVE=0')){throw 'Battle build diagnostics must match compilation capability'}
if(($battleCache -match 'LAGI_BATTLE_EXECUTE_A3:BOOL=ON') -ne $battleNinja.Contains('LAGI_BATTLE_BUILD_EXECUTE_A3=1')){throw 'Battle execution diagnostic must match the active option'}
if($normalNinja.Contains('LAGI_BATTLE_AUDIO_STACK=1')){throw 'Experimental audio stack leaked into default build'}
if(($battleCache -match 'LAGI_BATTLE_EXECUTE_A3:BOOL=ON') -ne $battleNinja.Contains('LAGI_BATTLE_AUDIO_STACK=1')){throw 'Audio stack must match explicit A3 execution option'}
if($normalNinja.Contains('lagi_battle_rays_vita.cpp.obj')){throw 'Battle-only ray service leaked into ordinary build'}
if($normalNinja.Contains('lagi_battle_graphics_vita.cpp.obj')){throw 'Battle-only graphics service leaked into ordinary build'}
if(!$battleNinja.Contains('lagi_battle_graphics_vita.cpp.obj')){throw 'Battle graphics service missing'}
if($normalNinja.Contains('lagi_resident_video_vita.cpp.obj')){throw 'Battle resident video service leaked into ordinary build'}
if(!$battleNinja.Contains('lagi_resident_video_vita.cpp.obj')){throw 'Resident video service missing'}
if($normalNinja.Contains('lagi_allocator_tables_vita.cpp.obj')){throw 'Battle allocator service leaked into ordinary build'}
if(!$battleNinja.Contains('lagi_allocator_tables_vita.cpp.obj')){throw 'Battle allocator table service missing'}
if(!$battleNinja.Contains('lagi_battle_damage_digits_vita.cpp.obj') -or $normalNinja.Contains('lagi_battle_damage_digits_vita.cpp.obj')){throw 'Floating battle digits must retain native-battle build isolation'}
$allocator=Get-Content -Raw "$battle/azel_upstream/mainMenuDebugTasks.cpp"
foreach($hook in @('lagiPushDramFileTable(pDramAllocator);','lagiPopDramFileTable(prev);','lagiResetDramFileTables();')){
 if(!$allocator.Contains($hook)){throw "Native allocator owner hook missing: $hook"}
}
$normalAllocator=Get-Content -Raw "$normal/azel_upstream/mainMenuDebugTasks.cpp"
if($normalAllocator.Contains('lagiPushDramFileTable')){throw 'Default allocator policy unexpectedly changed'}
$encounter=Get-Content -Raw "$battle/azel_upstream/field_fieldEncounterTask.cpp"
if(($encounter.Split('lagiRestoreBattleGraphics(pThis->m20)').Count-1) -ne 2){throw 'Both battle return restoration sites must be wired'}
if(!$encounter.Contains('if (!lagiPrepareBattleGraphicsRestore(pThis->m20)) return;')){throw 'Resource staging must precede sound cleanup'}
if(!$encounter.Contains('lagiDiscardBattleGraphics(pThis->m20);')){throw 'Encounter teardown leaks graphics backup'}
if(!$encounter.Contains('if (!pThis->m20)')){throw 'Encounter must not proceed after capture failure'}
$normalEncounter=Get-Content -Raw "$normal/azel_upstream/field_fieldEncounterTask.cpp"
if($normalEncounter.Contains('lagiCaptureBattleGraphics')){throw 'Ordinary encounter graphics path unexpectedly changed'}
$save=Get-Content -Raw "$battle/azel_upstream/kernel_loadSavegameScreen.cpp"
if(!$save.Contains('lagi::resident_video::save_scope(buffer,') -or !$save.Contains('lagi::resident_video::restore_scope(backup);')){throw 'Save/load byte backups must preserve resident ownership'}
$normalSave=Get-Content -Raw "$normal/azel_upstream/kernel_loadSavegameScreen.cpp"
if($normalSave.Contains('resident_video::')){throw 'Ordinary save/load path unexpectedly changed'}
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
 $execCache=Get-Content -Raw "$folder/CMakeCache.txt"
 $execEnabled=$execCache -match 'LAGI_BATTLE_EXECUTE_A3:BOOL=ON'
 if($execEnabled -ne $overlay.Contains('if (!lagiEnterNativeA3Battle(pThis)) return;')){throw 'A3 entry must match the explicit execution option'}
}
Write-Output "PASS: $($expanded.Count) shared/A3 objects plus native interpolation/monster dependencies compiled; ordinary build and dispatch/overlay safeguards retained. Not a retained-root link or hardware test."
