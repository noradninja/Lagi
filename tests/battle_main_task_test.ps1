param([string]$ScratchRoot='../battle-main-task-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/battleMainTask.cpp"
$begin=$source.IndexOf('struct sBattleMainTask :')
if($begin -lt 0){throw 'Native battle main task boundary changed'}
[IO.File]::WriteAllText("$scratch/native.inc",$source.Substring($begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_main_task_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native battle main task test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native battle main task failed'}
