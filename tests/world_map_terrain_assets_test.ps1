param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-terrain-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/world_map_terrain_assets_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'WORLD terrain asset fixture compile failed'}
& "$scratch/test.exe" (Resolve-Path -LiteralPath $DiscImage).Path
if($LASTEXITCODE){throw 'WORLD terrain asset fixture failed'}
