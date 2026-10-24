param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Native([long]$ea,[int]$bytes){$v=0L;for($i=0;$i -lt $bytes;++$i){$o=$ea+$i-0x6054000;$disc.Position=(258144+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short native read'};$v=($v-shl 8)-bor $b};return $v}
 foreach($p in @(@(0x606EA60,0x8502,2),@(0x606EA88,0x8504,2),@(0x606EB5C,0x0600675E,4),@(0x606EB60,0x060AB010,4))){if((Native $p[0] $p[2]) -ne $p[1]){throw ('Native rank identity contract differs {0:X8}' -f $p[0])}}
 foreach($p in @(@(0x606EC68,0x0484,2),@(0x606EC6A,0x00ff,2),@(0x606EC78,0x0024BCA0,4),@(0x606EBD2,0xEB0A,2),@(0x606EBEA,0x8B4F,2),@(0x606EC48,0x30D3,2),@(0x606EC8C,0x60D3,2))){if((Native $p[0] $p[2]) -ne $p[1]){throw ('Native record tail contract differs {0:X8}' -f $p[0])}}
} finally {$disc.Dispose()}
$repo=Split-Path $PSScriptRoot -Parent
$save=Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/loadSavegameScreen.cpp"
foreach($anchor in @('battleResults = saveDataBuffer.m394_battleResults;','saveDataBuffer.m394_battleResults = battleResults;','std::array<u8, 0x104> m394_battleResults;')){
 if(!$save.Contains($anchor)){throw 'Native saved-record ownership changed; re-audit serialization'}
}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rating-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/battle_rating_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Rank fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Rank fixture failed'}
