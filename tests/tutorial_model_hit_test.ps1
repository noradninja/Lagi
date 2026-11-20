param([string]$ScratchRoot='../tutorial-model-hit-test')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $root $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/tutorial_model_hit_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Tutorial model hit fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Tutorial model hit fixture failed'}
