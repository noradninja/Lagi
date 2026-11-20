param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$count){
  $v=0L;for($i=0;$i-lt$count;$i++){$o=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short native read'};$v=($v-shl8)-bor$b};return $v
 }
 foreach($pair in @(@(0x605417C,0x605637C),@(0x6056390,0x6058CCC),
  @(0x6058CCC,0x6055E08),@(0x6058CD4,0x6056288),@(0x6058CD8,0x6056368),
  @(0x6055EE4,0x6058B44),@(0x6055EEC,0x20000),@(0x6056374,0x6058CB8),
  @(0x605610C,0x601E25C),@(0x605622C,0x6058CAC),@(0x6056230,0x602E5CC))) {
  if((Read-Native 244259 0x6054000 $pair[0] 4)-ne$pair[1]){throw 'Native destination task service binding differs'}
 }
 foreach($pair in @(@(0x6055ECE,0x5CC),@(0x6055ED0,0x1E8),
  @(0x6055E42,0x70F8),@(0x6055E4A,0x2FD6), # pose05C4/color05CC; no animation
  @(0x6056080,0x62E3),@(0x6056088,0x7301),@(0x6056098,0x2382),
  @(0x60560A2,0x72FF),@(0x60560B0,0x2292),@(0x60560D0,0x0E35),
  @(0x60560EE,0x9306),@(0x60560FE,0x3CC),@(0x6056226,0xD000),
  @(0x6056228,0x5C0),@(0x605622A,0x1E4),@(0x605733A,0xE210),
  @(0x6057342,0x0316),
  @(0x6056198,0x8467),@(0x605619C,0x8456),@(0x60561A4,0x846B),@(0x60561A8,0x8457),
  @(0x60561CA,0x8468),@(0x60561D4,0x8469),@(0x60561F8,0x8469),@(0x60561FC,0x8451),
  @(0x6056204,0x846C),@(0x6056208,0x8452),@(0x6056248,0xD248),@(0x605624C,0xD148))) {
  if((Read-Native 244259 0x6054000 $pair[0] 2)-ne$pair[1]){throw 'Native destination task instruction/slot differs'}
 }
 foreach($slot in @(0x1D0,0x1D4,0x1D8,0x1DC,0x1E4,0x1E8,0x3CC,0x5AC,0x5B0,0x5B4,0x5B8,0x5C0,0x5C4,0x5CC)) {
  $offset=Read-Native 244271 0 $slot 4
  if($offset-le0-or$offset-ge212752){throw 'WORLDMAP model/pose slot outside original bundle'}
 }
}finally{$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-destinations-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_destinations_draw.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_destinations_draw_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Destination draw fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Destination draw fixture failed'}
