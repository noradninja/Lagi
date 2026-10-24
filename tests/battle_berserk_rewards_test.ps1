param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Native([long]$ea,[int]$bytes){$value=0L;for($i=0;$i -lt $bytes;++$i){$offset=$ea+$i-0x6054000;$disc.Position=(258144+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$value=($value-shl 8)-bor $b};return $value}
 foreach($p in @(@(0x606F30E,0xEC03,2),@(0x606F368,0x060AC094,4),@(0x606F446,0x02C8,2),
 @(0x606F380,0x7201,2),@(0x606F4A6,0x8533,2),@(0x606F4AA,0x8901,2),
 @(0x606F35C,0x060ABAD0,4),@(0x606F360,0x060ABA70,4),@(0x606F2A6,0xE304,2),
 @(0x606FC50,0xB9D7,2),@(0x606FC54,0xBB58,2),@(0x606FD30,0xBA9E,2))) {
  if((Native $p[0] $p[2]) -ne $p[1]){throw ('Native Berserk contract mismatch {0:X8}' -f $p[0])}
 }
 for($c=0;$c -lt 4;++$c){
  $table=Native (0x60AC094+$c*4) 4
  $entries=0
  while((Native ($table+$entries*8+6) 2) -ne 0){
   if(++$entries -gt 8){throw 'Native learned-list capacity exceeded'}
  }
 }
 'PASS native Berserk scan, proficiency address, result call order, announcement configuration and authored table capacity'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-berserk-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$common=Get-Content -Raw "$repo/extern/Azel/AzelLib/common.h"
$a=$common.IndexOf('    s8 getItemCount(eItems');$b=$common.IndexOf('    void addItemCount(eItems',$a)
if($a -lt 0 -or $b -le $a){throw 'Actual inventory method boundary changed'}
[IO.File]::WriteAllText("$scratch/inventory.inc",$common.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_berserk_rewards_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Berserk fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Berserk fixture failed'}
