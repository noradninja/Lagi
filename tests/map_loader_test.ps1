param([string]$ScratchRoot='..\map-loader-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_map_menu.cpp')
$begin=$source.IndexOf('bool lagiAzelLoadFieldMapResources(')
if($begin -lt 0){throw 'Native map loader source missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_loader_production.inc') -Value $source.Substring($begin)
$exe=Join-Path $scratch 'map_loader_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'map_loader_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Map loader host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Map loader host execution failed'}
