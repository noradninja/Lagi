param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build/world-garil')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$count) {
  $n=0L
  for($i=0;$i-lt$count;$i++) {
   $offset=$ea+$i-$base
   $disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $b=$disc.ReadByte(); if($b-lt0){throw 'Short disc read'}
   $n=($n-shl8)-bor$b
  }
  return $n
 }
 # Both bird initializers load the same story byte and MSB-first mask.
 foreach($pair in @(@(0x605B32C,0x604A402),@(0x605B330,0x6042F1A),@(0x605B680,0x604A402),@(0x605B684,0x6042F1A))) {
  if((Read-Native 244544 0x6054000 $pair[0] 4)-ne$pair[1]){throw 'Bird gate literal changed'}
 }
 foreach($pair in @(@(0x605B242,0xD33A),@(0x605B244,0xD13A),@(0x605B24A,0x2208),@(0x605B24C,0x8F05),@(0x605B58A,0xD33D),@(0x605B58C,0xD13D),@(0x605B592,0x2208),@(0x605B594,0x8F05))) {
  if((Read-Native 244544 0x6054000 $pair[0] 2)-ne$pair[1]){throw 'Bird gate instructions changed'}
 }
 $mask=Read-Native 58 0x6006000 0x6042F1A 1
 if($mask-ne2){throw 'Native bird mask changed'}
} finally {$disc.Dispose()}
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/field_field_a3_o_fld_a3_1.cpp"
if($generated.Contains('getBit(0xA * 8 + 1)')){throw 'Incorrect translated gate remains'}
if(([regex]::Matches($generated,'if \(!mainGameState.getBit\(0xA \* 8 \+ 6\)\)').Count)-ne2){throw 'Both generated gates must match native'}
for($byte=0;$byte-lt256;$byte++) {
 $nativeDisabled=($byte-band$mask)-eq0
 $adaptedDisabled=($byte-band(0x80-shr6))-eq0
 if($nativeDisabled-ne$adaptedDisabled){throw 'Gate parity failure'}
}
Write-Output 'PASS: both original bird gates and all 256 story-byte states; hardware interaction unverified.'
