param([string]$ScratchRoot='../map-town-availability-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_map_availability_vita.cpp"
$start=$source.IndexOf('bool lagiAzelResolveTownMapAvailability(')
if($start -lt 0){throw 'Town Map availability production boundary missing'}
Set-Content "$scratch/map_town_availability.inc" $source.Substring($start)
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/map_town_availability_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Town availability host compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Town availability host execution failed'}
