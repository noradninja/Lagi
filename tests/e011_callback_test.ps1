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
   0x06058068=0x62E2;0x06058074=0x8532;0x060580E0=0x06060E14}
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
 } finally {$stream.Dispose()}
}
