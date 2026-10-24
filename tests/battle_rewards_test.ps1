param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rewards-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Native([long]$ea,[int]$bytes){$value=0L;for($i=0;$i -lt $bytes;++$i){$offset=$ea+$i-0x6054000;$disc.Position=(258144+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$value=($value-shl 8)-bor $b};return $value}
 foreach($p in @(@(0x606ECB4,0x8502,2),@(0x606ECD6,0x8504,2),@(0x606ED44,0x060AAFD8,4),@(0x606ED48,0x060361F4,4),@(0x606ED4C,0x06035550,4),@(0x606ED22,0x3403,2),@(0x606ED5A,0x3453,2),@(0x606EE2A,0x0480,2),@(0x606EE2C,0x00F3,2),@(0x606EE3C,0x0604A66E,4))){if((Native $p[0] $p[2]) -ne $p[1]){throw ('Native reward contract mismatch {0:X8}' -f $p[0])}}
 if((Native 0x60AAFD8 4) -ne 0x60A9824){throw 'Native A3 reward table moved'}
 foreach($p in @(@(0x606EE34,0x060ABA00,4),@(0x606EE38,0x060ABAB0,4),@(0x606EA38,0x060353E6,4),@(0x606EA3C,0x060353FE,4),@(0x606EA40,0x0020C3F8,4),@(0x606E9AA,0x7302,2),@(0x606EDFA,0xE163,2))){if((Native $p[0] $p[2]) -ne $p[1]){throw ('Native reward announcement/cap contract mismatch {0:X8}' -f $p[0])}}
 $bytes=@(for($i=0;$i -lt 48;++$i){Native (0x60A9824+$i) 1})
 [IO.File]::WriteAllText("$scratch/native_reward_rows.inc",'static constexpr unsigned char nativeRows[]={'+($bytes -join ',')+'};')
} finally {$disc.Dispose()}
$common=Get-Content -Raw "$repo/extern/Azel/AzelLib/common.h"
$a=$common.IndexOf('    s8 getItemCount(eItems');$b=$common.IndexOf('    void addItemCount(eItems',$a)
if($a -lt 0 -or $b -le $a){throw 'Actual inventory method boundary changed'}
[IO.File]::WriteAllText("$scratch/inventory.inc",$common.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_rewards_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Reward fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Reward fixture failed'}
