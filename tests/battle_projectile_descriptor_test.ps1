param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$ScratchRoot='../battle-projectile-descriptor-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /EHsc /std:c++20 "/I$repo/src/integration" "$PSScriptRoot/battle_projectile_descriptor_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Projectile descriptor fixture compile failed'}
& "$scratch/test.exe" $DiscImage
if($LASTEXITCODE -ne 0){throw 'Projectile descriptor fixture failed'}
