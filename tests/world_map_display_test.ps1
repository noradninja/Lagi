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
 $callbacks=@(0x0605532A,0x0605568C,0x060556DA,0)
 for($i=0;$i -lt $callbacks.Count;$i++){
  if((Read-World (0x060587FC+$i*4) 4) -ne $callbacks[$i]){
   throw 'WORLD-local arrival task callback table differs'
  }
 }
 if((Read-World 0x0605692C 2) -ne 0xBE94 -or
    (Read-World 0x06056988 4) -ne 0x06056AD4 -or
    (Read-World 0x0605698C 4) -ne 0x06056DDE){
  throw 'WORLD environment setup or installed callbacks differ'
 }
 $arrivalCalls=@(
  @(0x06055690,0x060553F8), @(0x060556AA,0x0605532A),
  @(0x060556BA,0x06055606), @(0x060556BE,0x060555E2),
  @(0x060556C6,0x06055606), @(0x060556CA,0x060555E2),
  @(0x060556CE,0x060554FC)
 )
 foreach($call in $arrivalCalls){
  $opcode=Read-World $call[0] 2
  $displacement=$opcode -band 0xFFF
  if($displacement -ge 0x800){$displacement-=0x1000}
  if(($opcode -shr 12) -ne 0xB -or
     ($call[0]+4+2*$displacement) -ne $call[1]){
   throw 'WORLD arrival phase call differs'
  }
 }
 # Camera state 8 reads root +0x14, then waits while child +0x3D is zero.
 $arrivalGate=@(0xE03D,0x52D5,0x002C,0x2008,0x8973)
 for($i=0;$i -lt $arrivalGate.Count;$i++){
  if((Read-World (0x0605839A+$i*2) 2) -ne $arrivalGate[$i]){
   throw 'WORLD camera arrival wait differs'
  }
 }
 'PASS original WORLD display, arrival and environment callback addresses'
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
