param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Native([long]$ea,[int]$bytes){$v=0L;for($i=0;$i -lt $bytes;++$i){$o=$ea+$i-0x6054000;$disc.Position=(258144+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short native read'};$v=($v-shl 8)-bor $b};return $v}
 foreach($p in @(@(0x606EA60,0x8502,2),@(0x606EA88,0x8504,2),@(0x606EB5C,0x0600675E,4),@(0x606EB60,0x060AB010,4))){if((Native $p[0] $p[2]) -ne $p[1]){throw ('Native rank identity contract differs {0:X8}' -f $p[0])}}
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rating-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/battle_rating_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Rank fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Rank fixture failed'}
