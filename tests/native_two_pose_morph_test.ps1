param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Word([long]$ea){$v=0;for($i=0;$i -lt 2;$i++){$o=$ea+$i-0x06054000;$disc.Position=(258333+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$v=($v-shl 8)-bor $disc.ReadByte()};return $v}
 foreach($pair in @(@(0x06055BEC,0xD30D),@(0x06055C24,0x0605),@(0x06055C26,0x4E38),@(0x06054E46,0x5EF6),@(0x06054E48,0x4E11),@(0x06054E9C,0x0601),@(0x06054E9E,0x507A),@(0x060A9538,12),@(0x060A953A,0x1C0),@(0x060A9540,16),@(0x060A9542,0x1C4))){if((Read-Word $pair[0]) -ne $pair[1]){throw 'Native Arachnoth morph contract differs'}}
} finally {$disc.Dispose()}
$generated=Get-Content -Raw (Join-Path $repo "$BuildDirectory/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp")
if(!$generated.Contains('lagiNativeTwoPoseMorph(pThis->m98_poisonDart.m0')){throw 'Generated native morph call absent'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-pair-morph-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_native_two_pose_morph.inc"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/native_two_pose_morph_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Pair morph fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Pair morph fixture failed'}
