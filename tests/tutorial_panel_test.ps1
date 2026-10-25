param([string]$ScratchRoot='../tutorial-panel-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/tutorial_panel_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Tutorial panel compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Tutorial panel assertions failed'}
