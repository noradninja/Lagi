param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$audit=& "$repo/tools/audit_garil_scripts.ps1" -DiscImage $DiscImage -AsObject
$fades=@($audit.Calls | Where-Object NativeEA -in 0x0606FA5C,0x0606FAB4)
if($fades.Count-ne6-or@($fades|Where-Object {$_.Opcode-ne14-or$_.Arguments-ne1}).Count){throw 'Garil ordinary fade-action call sites differ'}
$stateActions=@($audit.Calls | Where-Object NativeEA -in 0x060728A4,0x060728E0,0x060728F8)
if($stateActions.Count-ne11-or@($stateActions|Where-Object {$_.Opcode-ne14-or$_.Arguments-ne0}).Count){throw 'Garil ordinary cutscene-state action sites differ'}
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
} finally {$disc.Dispose()}
'PASS original A5 fade/cutscene instructions and 17 ordinary-action sites; structural audit is incomplete at four unsupported entrypoints'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-garil-native-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_garil_script_native.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/garil_script_native_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Garil native fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Garil native fixture failed'}
