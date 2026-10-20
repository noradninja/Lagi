param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Word([long]$ea){$v=0;for($i=0;$i -lt 2;$i++){$o=$ea+$i-0x06054000;$disc.Position=(258333+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$v=($v-shl 8)-bor $b};return $v}
 foreach($pair in @(@(0x0605A230,0x9722),@(0x0605A236,0x6673),@(0x0605A23A,0x76BC),@(0x0605A278,0x210))){if((Read-Word $pair[0]) -ne $pair[1]){throw 'Native impact pose contract differs'}}
 $immediate=(Read-Word 0x0605A23A)-band 255
 if($immediate -ge 128){$immediate-=256}
 if(((Read-Word 0x0605A278)+$immediate) -ne 0x1CC){throw 'Signed SH-2 impact pose decode failed'}
} finally {$disc.Dispose()}
$generated=Get-Content -Raw (Join-Path $repo "$BuildDirectory/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp")
if(!$generated.Contains('lagiCreateBattleImpactModel(dramAllocatorEnd[8].mC_fileBundle, 0x18, 0x1CC, 0x210, position, rotation)')){throw 'Generated impact call differs from native pose contract'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-impact-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/battle_impact_model_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Impact fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Impact fixture failed'}
