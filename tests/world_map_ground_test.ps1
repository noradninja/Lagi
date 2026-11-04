param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Word([long]$ea){
  $offset=$ea-0x06054000
  $disc.Position=(244259+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
  return ($disc.ReadByte()-shl8)-bor$disc.ReadByte()
 }
 foreach($pair in @(@(0x0605594E,0xD42A),@(0x06055950,0xDC2A),
  @(0x06055952,0x5B41),@(0x06055958,0x6D42),@(0x0605595A,0x85D1),
  @(0x0605595C,0x65D1),@(0x06055960,0x4C0B),@(0x06055962,0x64E2),
  @(0x06055964,0x4B10),@(0x06055968,0x7D04),@(0x060559F8,0x0605),
  @(0x060559FA,0x881C),@(0x060559FC,0x0602),@(0x060559FE,0x11D2),
  @(0x0605598A,0x4408),@(0x0605598C,0x4400),@(0x06055994,0x3D2C),
  @(0x0605599A,0x3B2C),@(0x0605599E,0x6BBC),@(0x060559A0,0x5241),
  @(0x060559A6,0x85B1),@(0x060559A8,0x65B1),@(0x060559AE,0x64E2),
  @(0x060559B0,0x84D4),@(0x060559B6,0x70FF),@(0x060559B8,0x3037),
  @(0x060559C0,0x1E1A),@(0x060558F6,0x54E9),@(0x06055900,0xAF37))) {
  if((Word $pair[0])-ne$pair[1]){throw 'Original WORLD ground submission loop differs'}
 }
 'PASS original WORLD ground list/count, signed hierarchy/pose loads and native hierarchy-service call'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-ground-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_ground.cpp"
[IO.File]::WriteAllText("$scratch/production.inc",[regex]::Replace($source,'(?m)^#include[^\r\n]*',''))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_ground_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Ground fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Ground fixture failed'}
