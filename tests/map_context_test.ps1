param([string]$ScratchRoot='..\map-context-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_map_context.cpp')
$begin=$source.IndexOf('bool lagiAzelResolveCurrentMapContext(')
if($begin -lt 0){throw 'Native context service missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_context_production.inc') -Value $source.Substring($begin)
$exe=Join-Path $scratch 'map_context_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'map_context_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Map context compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Map context execution failed'}
