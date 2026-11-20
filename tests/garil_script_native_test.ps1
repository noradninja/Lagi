param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$audit=& "$repo/tools/audit_garil_scripts.ps1" -DiscImage $DiscImage -AsObject
$fades=@($audit.Calls | Where-Object NativeEA -in 0x0606FA5C,0x0606FAB4)
if($fades.Count-ne6-or@($fades|Where-Object {$_.Opcode-ne14-or$_.Arguments-ne1}).Count){throw 'Garil ordinary fade-action call sites differ'}
$stateActions=@($audit.Calls | Where-Object NativeEA -in 0x060728A4,0x060728E0,0x060728F8)
if($stateActions.Count-ne11-or@($stateActions|Where-Object {$_.Opcode-ne14-or$_.Arguments-ne0}).Count){throw 'Garil ordinary cutscene-state action sites differ'}
$placements=@($audit.Calls | Where-Object NativeEA -in 0x0605C020,0x0605C054)
if($placements.Count-ne2-or@($placements|Where-Object {$_.Opcode-ne14-or$_.Arguments-ne0}).Count){throw 'Garil placement action sites differ'}
$visibility=@($audit.Calls | Where-Object NativeEA -in 0x0607C82A,0x0607C86C)
if($visibility.Count-ne8-or@($visibility|Where-Object {$_.Opcode-ne14-or$_.Arguments-ne0}).Count){throw 'Garil visibility action sites differ'}
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Read-A5Word([long]$ea){
  $value=0L;for($i=0;$i-lt2;$i++){$offset=$ea+$i-0x06054000;$disc.Position=(244678+[long][Math]::Floor($offset/2048))*2352+16+$offset%2048;$b=$disc.ReadByte();if($b-lt0){throw 'Short A5 read'};$value=($value-shl8)-bor$b};return $value
 }
 foreach($word in @(@(0x0606FA68,0x03EC),@(0x0606FA6C,0x02EC),@(0x0606FA6E,0x3323),
  @(0x0606FA72,0xE210),@(0x0606FA78,0x0425),@(0x0606FA7C,0x0425),
  @(0x0606FA80,0x4300),@(0x0606FA8E,0x6503),@(0x0606FAA2,0x6503),
  @(0x0606FAC0,0x03EC),@(0x0606FAC4,0x02EC),@(0x0606FAC6,0x3323),
  @(0x0606FAD0,0x0425),@(0x0606FAD4,0x0425),@(0x0606FADE,0x4700),
  @(0x0606FAE2,0x06ED),@(0x0606FAF4,0x06ED))) {
  if((Read-A5Word $word[0])-ne$word[1]){throw 'Original A5 fade instruction differs'}
 }
 foreach($word in @(@(0x060728B0,0x2008),@(0x060728C0,0xE048),
  @(0x060728C4,0x2118),@(0x060728C8,0xE05C),@(0x060728CA,0x01EE),
  @(0x060728CC,0x2118),@(0x060728CE,0x8B04),@(0x060728D6,0xA8AE),
  @(0x060728EA,0xE100),@(0x060728F2,0xE05C),@(0x060728F6,0x0316),
  @(0x06072902,0xE101),@(0x0607290A,0xE05C),@(0x0607290E,0x0316),
  @(0x06071A4A,0xCB01),@(0x06071A4C,0x1305),@(0x06071A6A,0x0D36),
  @(0x06071A74,0xE05C),@(0x06071A76,0x0316),@(0x06071A78,0xE078),
  @(0x06071A7E,0x8B0E),@(0x06071A82,0xC802),@(0x06071A84,0x8905))) {
  if((Read-A5Word $word[0])-ne$word[1]){throw 'Original A5 cutscene-state instruction differs'}
 }
 foreach($word in @(@(0x0607FB12,0x1426),@(0x0607FB16,0x1437),@(0x0607FB1A,0x1428),
  @(0x0607C5A8,0x1E32),@(0x0607C5AC,0x1E23),@(0x0607C5B0,0x1E34),
  @(0x0607C5BC,0x1E28),@(0x0607C5C0,0x1E39),@(0x0607C5C6,0x1E2A),
  @(0x0607C5DE,0x1324),@(0x06069B00,0x9377),@(0x06069BF2,0x03E4),
  @(0x06069B1C,0x1E29),@(0x06069BCE,0x2E22),@(0x06069BD6,0x1E11),@(0x06069BDE,0x1E22),
  @(0x0605C088,0x005B),@(0x0605C08A,0x5000),@(0x0605C08C,0x0001),@(0x0605C08E,0x4000),
  @(0x0605C090,0xFFDD),@(0x0605C092,0xA000),@(0x0605C094,0x0800),
  @(0x0605C0A0,0x0048),@(0x0605C0A2,0),@(0x0605C0A4,0xFF54),@(0x0605C0A6,0))) {
  if((Read-A5Word $word[0])-ne$word[1]){throw 'Original A5 placement/camera instruction differs'}
 }
 foreach($word in @(@(0x0607C836,0xE301),@(0x0607C83E,0x0E34),
  @(0x0607C860,0x0249),@(0x0607C862,0x00B8),@(0x0607C868,0x0600),@(0x0607C86A,0xAE5A),
  @(0x0607C876,0xE300),@(0x0607C880,0x0E34),@(0x0607C95E,0x0249),@(0x0607C960,0x00B8),
  @(0x0607C974,0x0600),@(0x0607C976,0xAE4C))) {
  if((Read-A5Word $word[0])-ne$word[1]){throw 'Original A5 visibility/collision instruction differs'}
 }
} finally {$disc.Dispose()}
'PASS original A5 fade/cutscene/placement/visibility instructions and 27 ordinary-action sites; live light-wing side effects remain unsupported'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-garil-native-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_garil_script_native.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/garil_script_native_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Garil native fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Garil native fixture failed'}
$placement=Get-Content -Raw "$repo/src/integration/lagi_garil_placement.cpp"
$placement=[regex]::Replace($placement,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/placement.inc",$placement)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/garil_placement_test.cpp" "/Fe:$scratch/placement.exe" "/Fo:$scratch/placement.obj"
if($LASTEXITCODE){throw 'Garil placement fixture compile failed'}
& "$scratch/placement.exe"
if($LASTEXITCODE){throw 'Garil placement fixture failed'}
