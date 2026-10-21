param([string]$ScratchRoot='../e011-callback-test',[string]$DiscImage='',[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/src/integration" "$PSScriptRoot/e011_callback_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'E011 callback fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'E011 callback fixture failed'}
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/town_e011_twn_e011.cpp"
foreach($ea in @('0x605890C','0x605809E','0x60557E0','0x6055872')){
 if(!$generated.Contains("overlayScriptFunctions.m_oneArg[$ea]")){throw "Missing live E011 callback $ea"}
}
foreach($ea in @('0x60584E4','0x605851C','0x60579D2')){
 if(!$generated.Contains("overlayScriptFunctions.m_zeroArg[$ea]")){throw "Missing E011 completion callback $ea"}
}
if(!$generated.Contains('overlayScriptFunctions.m_threeArg[0x605948C]')){throw 'Missing native color fade callback'}
if(!$generated.Contains('sVec3_FP lagiTrackingPoint;')){throw 'Tracking point lacks native task ownership'}
if($DiscImage){
 $stream=[IO.File]::OpenRead($DiscImage)
 try {
  function Read-Original([long]$Address,[int]$Width){
   $value=0L
   for($i=0;$i -lt $Width;$i++){
    $offset=$Address+$i-0x06054000
    if($offset -lt 0 -or $offset -ge 58940){throw 'E011 read out of bounds'}
    $stream.Position=([long]218450+[int][Math]::Floor($offset/2048))*2352+16+($offset%2048)
    $byte=$stream.ReadByte();if($byte -lt 0){throw 'Short disc read'}
    $value=($value-shl 8)-bor $byte
   }
   return $value
  }
  $anchors=@{0x060588E2=0x52C2;0x060588E6=0x54C1;0x060588FC=0x3B17;
   0x06058C60=0x023D;0x06058FEC=0x06061F4C;0x06058FF0=0x06061F38;
   0x06058FF4=0x0602E1AA;0x06055840=0x0E46;0x060558CC=0x0E26;
   0x060558FC=0x25E1F000;0x06055904=0x060607D8;0x06055914=0x06060926;
   0x06058068=0x62E2;0x06058074=0x8532;0x060580E0=0x06060E14;
   0x0605949E=0xC801;0x060594B8=0xC802;0x06059538=0x06034C20;0x0605953C=0x06034AAA;
   0x060579DC=0x1522;0x060579E0=0x1513;0x0605853E=0xCB01}
  foreach($address in $anchors.Keys){$width=2;if($anchors[$address] -gt 65535){$width=4};if((Read-Original $address $width) -ne $anchors[$address]){throw ('Native E011 callback anchor changed at {0:X8}' -f $address)}}
  if((Read-Original 0x060600C4 4) -ne 0x0605FD10){throw 'Scheduler descriptor points elsewhere'}
  $counts=@{};$records=0;$previous=2147483647
  for($address=0x0605FD10;$address -le 0x060600B8;$address+=12){
   $threshold=Read-Original $address 4
   if($threshold -eq 0){break}
   if($threshold -gt $previous){throw 'Native schedule no longer descends'};$previous=$threshold
   $argument=Read-Original ($address+4) 4;$callback=Read-Original ($address+8) 4
   if($callback -notin @(0x060589E2,0x06058B2A,0x06058FF8)){throw ('Unrecovered scheduled callback {0:X8}' -f $callback)}
   if($argument -lt 0x06054000 -or $argument -ge 0x0606263C){throw 'Schedule argument outside overlay'}
   $counts[$callback]++;$records++
  }
  if($records -ne 78 -or $counts[[long]0x06058FF8] -ne 4){throw "Unexpected original schedule inventory: $records records"}
  Write-Output "PASS original E011 callback instructions and all $records schedule records including four ribbons"
  # Inventory native call records in the bounded script-only region. This is
  # data coverage, not a claim to execute all script branches on hardware.
  $known=@{};$arity=@{zeroArg=0;oneArg=1;oneArgPtr=1;twoArg=2;threeArg=3;fourArg=4}
  foreach($m in [regex]::Matches($generated,'overlayScriptFunctions\.m_(zeroArg|oneArgPtr|oneArg|twoArg|threeArg|fourArg)\[(0x[0-9a-fA-F]+)\]')){
   $key='{0}:{1:X8}' -f $arity[$m.Groups[1].Value],[Convert]::ToUInt32($m.Groups[2].Value.Substring(2),16);$known[$key]=$true
  }
  $town=Get-Content -Raw "$repo/extern/Azel/AzelLib/town/townScript.cpp"
  $town=$town.Substring($town.IndexOf('sKernelScriptFunctions gKernelScriptFunctions'))
  $town=$town.Substring(0,$town.IndexOf('sSaturnPtr callNativeWithArguments'))
  $n=-1
  foreach($line in ($town -split "`n")){
   if($line -match '//\s*(zero|one|two|three|four) arg'){$n=@{zero=0;one=1;two=2;three=3;four=4}[$Matches[1]]}
   if($line -match '\{(0x[0-9a-fA-F]+),\s*\{&'){$key='{0}:{1:X8}' -f $n,[Convert]::ToUInt32($Matches[1].Substring(2),16);$known[$key]=$true}
  }
  $calls=0
  for($address=0x0605415D;$address -lt 0x06054B75;$address++){
   if((Read-Original $address 1) -ne 7){continue}
   $n=Read-Original ($address+1) 1;if($n -gt 4){continue}
   $aligned=($address+5)-band(-4);$function=Read-Original $aligned 4
   if($function -lt 0x06006000 -or $function -ge 0x0606263C){continue}
   $key='{0}:{1:X8}' -f $n,$function
   if(!$known.ContainsKey($key)){throw ('Unregistered E011 script call {0} at {1:X8}' -f $key,$address)}
   $calls++
  }
  if($calls -lt 100){throw 'Unexpectedly sparse E011 script inventory'}
  Write-Output "PASS $calls original script native-call records have live kernel/overlay handlers of the correct arity"
 } finally {$stream.Dispose()}
}
