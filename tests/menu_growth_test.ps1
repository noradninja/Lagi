param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Word([long]$ea){$v=0;for($i=0;$i -lt 2;$i++){$o=$ea+$i-0x06006000;$disc.Position=(58+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$v=($v-shl 8)-bor $b};return $v}
 foreach($pair in @(@(0x0601A05E,0xE524),@(0x0601A070,0xE52C),@(0x0601A0B6,0xE52E),
  @(0x0601A082,0xE362),@(0x0601A138,0x0020),@(0x0601A13A,0x701C),
  @(0x0601A0E0,0xE48F),@(0x0601A0F0,0xE670),@(0x0601A25E,0x00C0),
  @(0x0601A268,0),@(0x0601A26A,0x8256),@(0x0602CC1E,0x1004),@(0x0602CC20,0x400))) {
  if((Read-Word $pair[0]) -ne $pair[1]){throw ('Native menu EXP contract differs at {0:X8}' -f $pair[0])}
 }
 'PASS original menu text positions, COMMON thresholds, level cap, polygon mode/color/coordinates'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-menu-growth-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/menu_growth_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Menu growth fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Menu growth fixture failed'}
