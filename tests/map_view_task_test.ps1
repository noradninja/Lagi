param([string]$ScratchRoot='..\map-view-task-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_map_view_vita.cpp')
$begin=$source.IndexOf('namespace {')
if($begin -lt 0){throw 'Native Map task production boundary missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_view_task_production.inc') -Value $source.Substring($begin)
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_books_resource.cpp')
$begin=$source.IndexOf('LagiAzelBooksIndexStep lagiAzelBooksStepIndex(')
$end=$source.IndexOf('LagiAzelBooksNavigationResult lagiAzelBooksNavigate(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Shared native index routine missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_view_index_production.inc') -Value $source.Substring($begin,$end-$begin)
$exe=Join-Path $scratch 'map_view_task_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'map_view_task_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Map view task compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Map view task execution failed'}
