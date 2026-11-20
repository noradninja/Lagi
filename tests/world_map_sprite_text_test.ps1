param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Read-Main([long]$ea,[int]$count){
  $value=0L
  for($i=0;$i-lt$count;$i++){
   $offset=$ea+$i-0x06006000
   $disc.Position=(58+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $byte=$disc.ReadByte();if($byte-lt0){throw 'Short MAIN read'}
   $value=($value-shl8)-bor$byte
  }
  return $value
 }
 foreach($literal in @(@(0x060437A0,0x06026316),@(0x0602651C,0x25E07000),
  @(0x06026520,0x25E00000),@(0x06026514,0x060251AC),
  @(0x06026518,0x06025334),@(0x0602652C,0x06025434))) {
  if((Read-Main $literal[0] 4)-ne$literal[1]){throw 'Native sprite-text service/table literal differs'}
 }
 foreach($word in @(
  @(0x060263C4,0x0110),@(0x060263C6,0x1000),@(0x060263C8,0x27C0),@(0x060263CA,0x0080),
  @(0x06026364,0x303C),@(0x0602636E,0x600B),@(0x06026372,0x3038),
  @(0x06026382,0x0237),@(0x06026386,0x4029),@(0x060263CC,0x4094),
  @(0x0602641A,0xE520),@(0x06026412,0xE702),@(0x0602645C,0x23A9),
  @(0x06026484,0x430B),@(0x06026486,0x34BC),@(0x06026496,0x7D20),
  @(0x060264B6,0x7D20),@(0x060264C0,0x7308),@(0x06026504,0x0FFF))) {
  if((Read-Main $word[0] 2)-ne$word[1]){throw 'Native sprite-text glyph/draw instruction differs'}
 }
 'PASS original MAIN sprite-text task, glyph staging, palette/size and depth-packet evidence'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-sprite-text-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_sprite_text.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_sprite_text_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'WORLD sprite-text fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'WORLD sprite-text fixture failed'}
