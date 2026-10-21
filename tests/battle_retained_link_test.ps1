param([string]$BattleBuild='build-battle-dev',[string]$Nm='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$build=Join-Path $repo $BattleBuild
$cache=Get-Content -Raw "$build/CMakeCache.txt"
if($cache -notmatch 'LAGI_BATTLE_NATIVE_BUILD:BOOL=ON' -or $cache -notmatch 'LAGI_BATTLE_LINK_AUDIT:BOOL=ON'){throw 'Both battle compilation and retained-root audit must be enabled'}
if(!$Nm) {
 if($env:VITASDK){$Nm=Join-Path $env:VITASDK 'bin/arm-vita-eabi-nm.exe'}
 else{$Nm=(Get-Command arm-vita-eabi-nm -ErrorAction Stop).Source}
}
$symbols=(& $Nm -C --defined-only "$build/lagi") -join "`n"
if($LASTEXITCODE -ne 0){throw 'Cannot inspect battle ELF'}
$required=@('overlayStart_BTL_A3','createBattleEngineTask','battleEngine_CreateHud1',
 'battleEngine_CreateRadar','createBattleCommandMenu','createBattleResultScreen',
 'displayRaySegment','sendRaySegmentToVdp1','isSoundFileLoaded')
if($cache -match 'LAGI_BATTLE_EXECUTE_A3:BOOL=ON'){$required+=@('lagiPrepareNativeA3Battle','lagiEnterNativeA3Battle','BTL_GenericData::staticInit')}
foreach($name in $required) {
 if($symbols -notmatch ('(?m)^[0-9a-fA-F]+\s+[Tt]\s+'+[regex]::Escape($name)+'\(')){
  throw "Expected retained native code missing: $name"
 }
}
Write-Output 'PASS: retained-root ELF contains native A3 entry, engine, HUD/radar/commands/results and restored ray/resource code. Link evidence only, not battle execution.'
