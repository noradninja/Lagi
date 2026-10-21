param([string]$ScratchRoot='../model-snapshot-cache-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/model_snapshot_cache_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Model snapshot cache test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Model snapshot cache test failed'}
