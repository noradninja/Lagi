param([Parameter(Mandatory=$true)][string]$Build,[switch]$Enabled)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$directory=Join-Path $repo $Build
$cache=Get-Content -Raw (Join-Path $directory 'CMakeCache.txt')
$generated=Get-Content -Raw (Join-Path $directory 'azel_upstream/mainMenuDebugTasks.cpp')
$ninja=Get-Content -Raw (Join-Path $directory 'build.ninja')
$world=$cache-match '(?m)^LAGI_WORLD_MAP_BRINGUP:BOOL=ON\r?$'
$execute=$cache-match '(?m)^LAGI_BATTLE_EXECUTE_A3:BOOL=ON\r?$'
if(($world-and$execute)-ne[bool]$Enabled){throw 'Configured WORLD/A3 options do not match expected A5 selection'}
$dispatch=$generated.Contains('gFieldOverlayFunction = overlayStart_FLD_A5;')
$objects=[regex]::Matches($ninja,'(?m)^build CMakeFiles/lagi\.dir/azel_upstream/field_field_a5_[^ :]+\.cpp\.obj:')
if($dispatch-ne[bool]$Enabled){throw 'Generated A5 field dispatch does not match build gate'}
$sources=@(Get-ChildItem -LiteralPath "$repo/extern/Azel/AzelLib/field/field_a5" -Filter '*.cpp')
if($Enabled){
 $shared=Get-Content -Raw (Join-Path $directory 'azel_upstream/field_field_a3_o_fld_a3.cpp')
 if(!$shared.Contains('if (lagiAzelTryGarilScriptAction(readSaturnEA(r14), readSaturnS32(r14 + 4), m54_currentResult)) break;')-or
    !$shared.Contains('if (lagiAzelTryGarilScriptAction(readSaturnEA(r14), m54_currentResult)) break;')-or
    !$ninja.Contains('build CMakeFiles/lagi.dir/src/integration/lagi_garil_script_native.cpp.obj:')){
  throw 'Owned Garil native-action binding missing'
 }
 if(!$ninja.Contains('build CMakeFiles/lagi.dir/azel_upstream/field_fieldModelRender.cpp.obj:')){
  throw 'Native A5 model/LCS context dependency missing'
 }
 if(!$ninja.Contains('build CMakeFiles/lagi.dir/azel_upstream/field_fieldAnimRingSubTask.cpp.obj:')){
  throw 'Native A5 exit animation dependency missing'
 }
 if($objects.Count-ne$sources.Count){throw 'Native A5 module selection is incomplete'}
 foreach($source in $sources){
  $target='build CMakeFiles/lagi.dir/azel_upstream/field_field_a5_'+$source.Name+'.obj:'
  if(!$ninja.Contains($target)){throw "Missing A5 target: $($source.Name)"}
 }
}elseif($objects.Count-or$ninja.Contains('build CMakeFiles/lagi.dir/src/integration/lagi_garil_script_native.cpp.obj:')){throw 'A5 modules leaked into the disabled WORLD configuration'}
Write-Output "PASS WORLD/A5 build gate: enabled=$([bool]$Enabled) nativeModules=$($objects.Count); not a link or hardware test"
