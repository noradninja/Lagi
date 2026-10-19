param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev',[string]$ScratchRoot='../arachnoth-music-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot));New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_2_BTL_A3_2.cpp"
if(!$source.Contains('&lagiArachnothInitMusic, battle_A3_2_func0') -or $source.Contains('&battle_A3_initMusic, battle_A3_2_func0')){throw 'Boss still uses field-battle audio initializer'}
$stream=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$width){
  $v=0L
  for($i=0;$i -lt $width;$i++){$off=$ea+$i-$base;$stream.Position=($lba+[long][Math]::Floor($off/2048))*2352+16+($off%2048);$b=$stream.ReadByte();if($b -lt 0){throw 'Short disc read'};$v=($v-shl 8)-bor $b}
  $v
 }
 if((Read-Native 258333 0x6054000 0x6054016 2) -ne 0xE443 -or (Read-Native 258333 0x6054000 0x605406C 4) -ne 0x0602B400){throw 'Original boss bank/loadSoundBanks anchors changed'}
 $configs=Read-Native 182 0x200000 (0x2152D8+67*16+4) 4
 if((Read-Native 182 0x200000 ($configs+18*4) 4) -ne 0x03110006){throw 'Original Charge effect mapping changed'}
 'PASS original boss selects bank 67; Charge 118 maps to data bank 3, sound 17, control 6, not music control 0'
} finally {$stream.Dispose()}
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/src/integration" "$PSScriptRoot/arachnoth_music_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Music fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Music fixture failed'}
