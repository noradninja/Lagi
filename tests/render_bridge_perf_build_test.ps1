param([string]$Build='build-battle-dev',[bool]$ExpectedOn=$true)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$rows=@(& "$repo/tools/audit_vita_perf_build.ps1" -Build $Build)
$bridge=$rows|Where-Object Source -eq RenderBridge
if(!$bridge.Found -or $bridge.NDebug -or $bridge.ExplicitFastMath){throw 'Bridge missing or unsafe assertion/math flags'}
if($ExpectedOn){
 if($bridge.Optimization -ne '-O2'){throw 'Bridge optimization missing'}
 foreach($flag in @('-fwrapv','-fno-strict-aliasing','-fno-fast-math','-ffp-contract=off')){
  if(!$bridge.Flags.Contains($flag)){throw "Bridge safety flag missing: $flag"}
 }
}elseif($bridge.Optimization -ne 'compiler default (no -O flag)'){
 throw 'Disabled experiment retained optimization flags'
}
# These bring-up units must not silently join this one-file experiment.
foreach($name in @('Runtime','TaskHost','Native3D','Animation','BattleEngine','BroodQueen','Urchin')){
 $row=$rows|Where-Object Source -eq $name
 if($row.Found -and ($row.Optimization -ne 'compiler default (no -O flag)' -or $row.NDebug)){
  throw "Unrelated unit changed optimization/assertions: $name"
 }
}
Write-Output "PASS: render-bridge experiment enabled=$ExpectedOn, strict flags/assertions, unrelated units unchanged. Not hardware validation."
