param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$bytes){
  $value=0L
  for($i=0;$i -lt $bytes;$i++){
   $offset=$ea+$i-0x06054000
   $disc.Position=(258333+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $byte=$disc.ReadByte();if($byte -lt 0){throw 'Short disc read'}
   $value=($value-shl 8)-bor $byte
  }
  return $value
 }
 foreach($pair in @(
  @(0x06058C8A,0xE701,2),@(0x06058D46,0x1000,2),@(0x06058D6C,0x0605A4EC,4),
  @(0x0605A634,0x08000000,4),@(0x0605A63C,0xF8000000L,4),@(0x0605A640,0x0FFFFFFF,4),
  @(0x0605A628,0x060A9624,4),@(0x0605A608,0xE278,2),
  @(0x060A9B54,0x0605A654,4),@(0x060A9B58,0x0605A736,4),
  @(0x0605A758,0x060A0DE4,4),@(0x060A0E0C,0x0602E03C,4),@(0x060A10E4,0x0602DF98,4)
 )){
  if((Read-Native $pair[0] $pair[2]) -ne $pair[1]){throw ('Native bile/trail contract differs at {0:X8}' -f $pair[0])}
 }
 'PASS original death caller, random cone, lifetime, descriptor and plain/Gouraud trail dispatch'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-bile-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
foreach($test in @('battle_trail_draw_test','arachnoth_death_bile_test')){
 & cl /nologo /std:c++20 /EHsc "$PSScriptRoot/$test.cpp" "/Fe:$scratch/$test.exe" "/Fo:$scratch/$test.obj"
 if($LASTEXITCODE -ne 0){throw 'Bile/trail fixture compile failed'}
 & "$scratch/$test.exe"
 if($LASTEXITCODE -ne 0){throw 'Bile/trail fixture failed'}
}
