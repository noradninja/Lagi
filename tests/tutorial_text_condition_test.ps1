param([string]$ScratchRoot='../tutorial-text-condition-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/tutorial_text_condition_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Tutorial condition compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Tutorial condition assertions failed'}
