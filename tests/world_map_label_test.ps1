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
  @(0x060574FE,0x5449),@(0x06057514,0x54D6),@(0x06057520,0x54D7),
  @(0x0605752E,0x3238),@(0x06057534,0x73FD),
  @(0x0605753A,0x3567),@(0x06057540,0x3523),@(0x06057546,0x3167),@(0x0605754C,0x3323),
  @(0x06057554,0x1E21),@(0x06057558,0x1E32),
  @(0x0605756C,0x4300),@(0x0605756E,0x633B),@(0x06057570,0x335C),
  @(0x06057582,0xE568),@(0x0605758E,0xE5A8),@(0x0605759E,0x1E43),
  @(0x060575B0,0x0200),@(0x060575B2,0x0333),@(0x060575B4,0xFE00),
  @(0x060575B6,0xFF00),@(0x060575B8,0xFF60),@(0x060575BA,0x00A0))) {
  if((Read-World $word[0] 2)-ne$word[1]){throw 'WORLD label bounds instruction/literal differs'}
 }
 foreach($literal in @(@(0x060575C0,0x06036034),@(0x060575C4,0x10000),
                       @(0x060575C8,0x06035664),@(0x060575CC,0x0603563C),
                       @(0x060576FC,0x060256A8),@(0x06057700,0x25C7B600),
                       @(0x06057704,0x060263D4))) {
  if((Read-World $literal[0] 4)-ne$literal[1]){throw 'WORLD label projection service differs'}
 }
 'PASS original WORLD label transform/projection and signed bounds evidence'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-label-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_label.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_label_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'WORLD label fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'WORLD label fixture failed'}
