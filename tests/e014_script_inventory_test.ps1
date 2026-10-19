param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/$Build/azel_upstream/town_e014_twn_e014.cpp"
$dispatch=Get-Content -Raw "$repo/$Build/azel_upstream/town_town.cpp"
if(!$dispatch.Contains('gFieldOverlayFunction = overlayStart_TWN_E014;')){throw 'Post-boss E014 dispatch missing'}
$stream=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Extent([long]$lba,[int]$length){
  $bytes=New-Object byte[] $length
  for($at=0;$at -lt $length;$at+=2048){
   $stream.Position=($lba+[long][Math]::Floor($at/2048))*2352+16
   $count=[Math]::Min(2048,$length-$at)
   if($stream.Read($bytes,$at,$count) -ne $count){throw 'Short disc read'}
  }
  return ,$bytes
 }
 $pvd=Read-Extent 16 2048
 $root=Read-Extent ([BitConverter]::ToUInt32($pvd,158)) ([BitConverter]::ToUInt32($pvd,166))
 $program=$null
 for($at=0;$at -lt $root.Length;){
  $length=$root[$at];if(!$length){$at=([int][Math]::Floor($at/2048)+1)*2048;continue}
  $name=[Text.Encoding]::ASCII.GetString($root,$at+33,$root[$at+32]).Split(';')[0]
  if($name -eq 'TWN_E014.PRG'){
   $program=Read-Extent ([BitConverter]::ToUInt32($root,$at+2)) ([BitConverter]::ToUInt32($root,$at+10));break
  }
  $at+=$length
 }
 if(!$program){throw 'Missing original E014 overlay'}
 function BE32([int]$at){return ([long]$program[$at]-shl 24)-bor ([long]$program[$at+1]-shl 16)-bor ([long]$program[$at+2]-shl 8)-bor $program[$at+3]}
 # Original restore-main-logic stores the Update/Draw callbacks; close-cutscene
 # retires the streaming task. Verify instruction anchors, not just aliases.
 if((BE32 0x2C7C) -ne 0x1522000B -or (BE32 0x37DC) -ne 0x5035CB01){throw 'E014 completion callback instruction anchors changed'}
 $known=@{};$arity=@{zeroArg=0;oneArg=1;oneArgPtr=1;twoArg=2;threeArg=3;fourArg=4}
 foreach($m in [regex]::Matches($source,'overlayScriptFunctions\.m_(zeroArg|oneArgPtr|oneArg|twoArg|threeArg|fourArg)\[(0x[0-9a-fA-F]+)\]')){
  $known[('{0}:{1:X8}' -f $arity[$m.Groups[1].Value],[Convert]::ToUInt32($m.Groups[2].Value.Substring(2),16))]=$true
 }
 $kernel=Get-Content -Raw "$repo/extern/Azel/AzelLib/town/townScript.cpp"
 $kernel=$kernel.Substring($kernel.IndexOf('sKernelScriptFunctions gKernelScriptFunctions'))
 $kernel=$kernel.Substring(0,$kernel.IndexOf('sSaturnPtr callNativeWithArguments'))
 $n=-1
 foreach($line in ($kernel -split "`n")){
  if($line -match '//\s*(zero|one|two|three|four) arg'){$n=@{zero=0;one=1;two=2;three=3;four=4}[$Matches[1]]}
  if($line -match '\{(0x[0-9a-fA-F]+),\s*\{&'){$known[('{0}:{1:X8}' -f $n,[Convert]::ToUInt32($Matches[1].Substring(2),16))]=$true}
 }
 $calls=0;$missing=0
 # Bounded prefix before the background-task code; record inventory only,
 # not proof of complete script execution or branch coverage.
 for($at=0x15D;$at -lt 0x800;$at++){
  if($program[$at] -ne 7 -or $program[$at+1] -gt 4){continue}
  $n=$program[$at+1];$function=BE32 (($at+5)-band(-4))
  if($function -lt 0x06006000 -or $function -ge 0x06054000+$program.Length){continue}
  $key='{0}:{1:X8}' -f $n,$function
  if(!$known.ContainsKey($key)){Write-Output ('Missing {0} at {1:X8}' -f $key,($at+0x06054000));$missing++}
  $calls++
 }
 if($missing){throw "$missing unregistered E014 prefix calls"}
 if(!$calls){throw 'No E014 script records found'}
 "PASS: E014 native dispatch and $calls original prefix native-call records; data inventory, not cinematic execution"
} finally {$stream.Dispose()}
