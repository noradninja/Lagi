param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Read-World([long]$ea,[int]$count){
  $value=0L
  for($i=0;$i-lt$count;$i++){
   $offset=$ea+$i-0x06054000
   $disc.Position=(244259+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $byte=$disc.ReadByte();if($byte-lt0){throw 'Short WORLD read'}
   $value=($value-shl8)-bor$byte
  }
  return $value
 }
 foreach($word in @(
  @(0x0605537C,0x53EE),@(0x0605537E,0x4300),@(0x06055380,0x5208),
  @(0x06055384,0x6131),@(0x06055392,0x7428),@(0x0605539A,0x7478),
  @(0x06055606,0xE03E),@(0x0605561A,0x8552),@(0x06055622,0x71FF),
  @(0x06055628,0x50E3),@(0x0605562A,0x6303),@(0x0605562E,0x4008),
  @(0x06055634,0x303C),@(0x06055636,0x4008),@(0x0605563A,0x7201),
  @(0x06055642,0x6551),@(0x06055644,0x3253),@(0x06055648,0x1421),
  @(0x0605564C,0x1421),@(0x06055654,0x7428),@(0x06055660,0x7478),
  @(0x060556E4,0x62D2),@(0x060556E6,0x6122),@(0x060556F0,0x3207),
  @(0x060556F6,0xE200),@(0x06055700,0x7314),@(0x06055702,0x7708),
  @(0x06055708,0x56E6),@(0x0605570A,0x55E5),@(0x0605570E,0x7428))) {
  if((Read-World $word[0] 2)-ne$word[1]){throw ('WORLD arrival instruction differs at {0:X8}'-f$word[0])}
 }
 foreach($literal in @(@(0x060553EC,0x06021FFA),@(0x060553F0,0x06021690),
  @(0x060553F4,0x060097E6),@(0x06055680,0x206AC4),@(0x06055684,0x06021798),
  @(0x06055688,0x06009B84),@(0x06055730,0x64000),@(0x06055734,0x06009BB6))) {
  if((Read-World $literal[0] 4)-ne$literal[1]){throw 'WORLD arrival service literal differs'}
 }
 'PASS original WORLD arrival animation, native service calls, descriptor stride and model/shadow draw evidence'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-arrival-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_arrival.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_arrival_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'WORLD arrival fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'WORLD arrival fixture failed'}
