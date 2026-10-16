param([string]$ScratchRoot='../map-parent-task-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_map_parent_vita.cpp"
$begin=$source.IndexOf('namespace {')
if($begin -lt 0){throw 'Map parent production boundary missing'}
[IO.File]::WriteAllText("$scratch/map_parent.inc",$source.Substring($begin))
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/map_parent_task_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Map parent host compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Map parent assertions failed'}
