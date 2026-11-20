param([string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$generated=Join-Path (Join-Path $repo $BuildDirectory) 'azel_upstream'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-reward-save-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
function Section([string]$text,[string]$begin,[string]$end){
 $a=$text.IndexOf($begin);$b=$text.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Source boundary changed: $begin"}
 return $text.Substring($a,$b-$a)
}
$common=Get-Content -Raw "$generated/common.h"
$stats=Section $common 'struct s_gameStats' 'struct sBitfieldMapEntry'
$state=Section $common 'struct s_mainGameState' 'extern s_mainGameState mainGameState;'
$module=Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/moduleManager.h"
$status=Section $module 'struct sSaveGameStatus' 'extern sSaveGameStatus gSaveGameStatus;'
[IO.File]::WriteAllText("$scratch/reward_save_state.inc",$stats+$state+$status)
$save=Get-Content -Raw "$generated/kernel_loadSavegameScreen.cpp"
$functions=Section $save 'struct sSaveDataRaw' 'struct sLoadSavegameScreen :'
[IO.File]::WriteAllText("$scratch/reward_save_functions.inc",$functions)
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/reward_save_roundtrip_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Reward save fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Reward save fixture failed'}
