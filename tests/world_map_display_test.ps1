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
 # Arrival uses horizontal distance and target-minus-position heading.
 # Its rotated displacement is added, and movement still runs after +3D is set.
 foreach($word in @(
  @(0x06055464,0x53F1),@(0x06055466,0x1F34),
  @(0x0605548A,0x53F5),@(0x0605548C,0x55F2),@(0x0605548E,0x3538),
  @(0x06055490,0x53F3),@(0x06055494,0x64F2),@(0x06055496,0x3438),
  @(0x060554E6,0x312C),@(0x060554E8,0x2312),
  @(0x060554F0,0x312C),@(0x060554F2,0x1312),
  @(0x06055536,0xE001),@(0x0605553E,0x2100),
  @(0x06055570,0x56EA),@(0x06055574,0x55E9),@(0x06055576,0xBF97),
  @(0x060555D0,0x514D),@(0x060555D2,0x5048),@(0x060555D4,0x3013),
  @(0x060555DA,0x5046),@(0x060555DC,0x5049),
  @(0x060555EA,0x2F36),@(0x060555EC,0xBFF0),@(0x060555FA,0x54E6),@(0x060555FC,0x1E06)
 )){
  if((Read-World $word[0] 2)-ne $word[1]){throw 'WORLD arrival distance/movement instruction differs'}
 }
 foreach($literal in @(@(0x060553E4,0x238E38E),@(0x06055670,0x12345),
                        @(0x06055674,0x38E38E),@(0x06055678,0x06055B42))) {
  if((Read-World $literal[0] 4)-ne$literal[1]){throw 'WORLD arrival heading literal differs'}
 }
 if((Read-World 0x060553B0 2)-ne0x1000-or(Read-World 0x0605566E 2)-ne0x1999){
  throw 'WORLD arrival heading threshold/gain differs'
 }
 # Native map-table switch: original story gate, selection slots and targets.
 foreach($word in @(
  @(0x06057CE2,0xE01A),@(0x06057CEE,0x8456),@(0x06057CF0,0x2308),
  @(0x06057CF8,0x741B),@(0x06057CFE,0x7604),@(0x06057D02,0x2328),
  @(0x06057D16,0xE1BF),@(0x06057D22,0xE221),
  @(0x06057D4C,0x00DE),@(0x06057D4E,0x0087),
  @(0x06057D7E,0xE240),@(0x06057D8A,0xE104),@(0x06057D9C,0xE200))) {
  if((Read-World $word[0] 2)-ne$word[1]){throw 'WORLD map-table switch instruction differs'}
 }
 if((Read-World 0x06057D64 4)-ne0x06059688-or(Read-World 0x06057DB0 4)-ne0x06059B2C){
  throw 'WORLD ordinary/fleet table literals differ'
 }
 'PASS original WORLD display, arrival, table-switch and environment evidence'
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
