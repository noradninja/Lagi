param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$count){
  $out=New-Object byte[] $count
  for($i=0;$i -lt $count;$i++){$offset=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$out[$i]=$b};return ,$out
 }
 # Native draw call table: translate, yaw, pop/push, X/Y/Z scale.
 foreach($entry in @(@(0x6055234,0x6035C48),@(0x6055238,0x6035E64),@(0x6055240,0x6035C38),@(0x6055244,0x6035BC0),@(0x6055248,0x6035FA4),@(0x605524C,0x6035FD4),@(0x6055250,0x6036004))){
  $raw=Read-Native 244259 0x6054000 $entry[0] 4
  $value=0L;foreach($b in $raw){$value=($value-shl 8)-bor $b}
  if($value -ne $entry[1]){throw 'Original WORLD draw call table differs'}
 }
 foreach($slot in @(0x1EC,0x5C8,0x5D0)){
  $raw=Read-Native 244271 0 $slot 4
  $offset=0L;foreach($b in $raw){$offset=($offset-shl 8)-bor $b}
  if($offset -le 0 -or $offset -ge 212752){throw 'Original map bundle resource slot invalid'}
 }
 'PASS original WORLD draw calls and WORLDMAP.MCB resource slots; no game bytes written'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-model-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_model.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_model_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'World model fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'World model fixture failed'}
