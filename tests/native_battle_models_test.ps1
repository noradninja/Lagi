param([string]$ScratchRoot='../native-battle-models-test')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $root $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$root/include" "$PSScriptRoot/native_battle_models_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Native model fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Native model fixture failed'}
