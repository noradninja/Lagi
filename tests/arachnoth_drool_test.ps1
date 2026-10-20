param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Word([long]$ea){$v=0;for($i=0;$i -lt 2;$i++){$o=$ea+$i-0x06054000;$disc.Position=(258333+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$v=($v-shl 8)-bor $b};return $v}
 foreach($pair in @(@(0x0605B466,0x108),@(0x0605B468,0x2EEC),@(0x0605B46A,0x510),@(0x0605B46C,0xCD0),@(0x0605B46E,0x400),@(0x0605B470,0x0602),@(0x0605B472,0xE136))){if((Read-Word $pair[0]) -ne $pair[1]){throw 'Native strand draw contract differs'}}
 'PASS original strand draw constants and MAIN 0602E136 dispatch'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-drool-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_arachnoth_drool.inc"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/arachnoth_drool_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Strand fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Strand fixture failed'}
