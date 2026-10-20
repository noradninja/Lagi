param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-World([long]$ea,[int]$count){
  $value=0L
  for($i=0;$i -lt $count;$i++){$offset=$ea+$i-0x06054000;$disc.Position=(244259+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$value=($value-shl 8)-bor $b};return $value
 }
 $values=@(0xA3,0x2000,0xA3D,0xF5,0x4000,0x1999,0x199,0x6000,0x3333,0x333,0x1000,0x51E)
 for($i=0;$i -lt $values.Count;$i++){
  if((Read-World (0x0605428E+$i*2) 2) -ne $values[$i]){throw 'Native easing profile table differs'}
 }
 if((Read-World 0x060542AC 4) -ne 0x3E7000){throw 'Native easing maximum differs'}
 if((Read-World 0x06054190 4) -ne 0x0605571A){throw 'Native root child initializer differs'}
 'PASS original WORLD easing profiles and WORLD-local root +14 initializer'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-display-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_display.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_display_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'World display fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'World display fixture failed'}
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/world_map_motion_test.cpp" "/Fe:$scratch/motion.exe" "/Fo:$scratch/motion.obj"
if($LASTEXITCODE -ne 0){throw 'World motion fixture compile failed'}
& "$scratch/motion.exe"
if($LASTEXITCODE -ne 0){throw 'World motion fixture failed'}
