param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Read-A5Word([long]$ea){
  $value=0L;for($i=0;$i-lt2;$i++){$offset=$ea+$i-0x06054000;$disc.Position=(244678+[long][Math]::Floor($offset/2048))*2352+16+$offset%2048;$b=$disc.ReadByte();if($b-lt0){throw 'Short A5 read'};$value=($value-shl8)-bor$b};return $value
 }
 foreach($word in @(@(0x06070652,0x053A),@(0x06070B4C,0x51DF),@(0x06070B56,0x8804),
  @(0x06070B5E,0x64E0),@(0x06070B60,0x4415),@(0x06070B64,0x7E01),
  @(0x06070B70,0x2EC9),@(0x06070B72,0x4408),@(0x06070B76,0x3E4C),
  @(0x06070B7E,0x67E0),@(0x06070B80,0x2778),@(0x06070B84,0x7E01),
  @(0x06070B94,0x2F46),@(0x06070B96,0x7654),@(0x06070B9A,0x753C),
  @(0x06070D1C,0x0601),@(0x06070D1E,0xCB4E),
  @(0x06054496,0xE416),@(0x0605464E,0xE417),@(0x0605482A,0xE418),
  @(0x0607A9AC,0xD31C),@(0x0607AA20,0x0607),@(0x0607AA22,0x0D94),
  @(0x06070DD2,0x887F),@(0x06070DE4,0x7414),@(0x06070DEC,0x8801),
  @(0x060619CC,0x1E03))) {
  if((Read-A5Word $word[0])-ne$word[1]){throw ('A5 choice/metadata evidence differs at {0:X8}'-f$word[0])}
 }
} finally {$disc.Dispose()}
$audit=& "$repo/tools/audit_garil_scripts.ps1" -DiscImage $DiscImage -AsObject
if($audit.Unknown.Count-or$audit.Metadata.Count-ne3){throw 'A5 structural audit classification differs'}
for($i=0;$i-lt3;$i++){
 if($audit.Metadata[$i].Index-ne(0x16+$i)-or$audit.Metadata[$i].Records-ne@(2,3,3)[$i]){throw 'A5 encounter record inventory differs'}
}
'PASS original A5 choice30 dispatch/state instructions and native radar metadata consumers; structural audit has no undecoded paths, not gameplay evidence'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-garil-choice-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_garil_script_choice.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/choice.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/garil_script_choice_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Garil choice fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Garil choice fixture failed'}
