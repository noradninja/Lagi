param([string]$ScratchRoot='../map-confirmation-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_map_confirmation_vita.cpp"
$start=$source.IndexOf('namespace {')
if($start -lt 0){throw 'Confirmation task production boundary missing'}
Set-Content "$scratch/map_confirmation_production.inc" $source.Substring($start)
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/map_confirmation_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Confirmation host compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Confirmation host execution failed'}
