param([string]$ScratchRoot='../battle-task-heap-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$heap=Get-Content -Raw "$repo/src/integration/lagi_heap_vita.cpp"
$begin=$heap.IndexOf('static const u32 HEAP_MAGIC')
if($begin -lt 0){throw 'Heap service boundary changed'}
[IO.File]::WriteAllText("$scratch/heap.inc",$heap.Substring($begin))
$task=Get-Content -Raw "$repo/src/integration/lagi_task_vita.cpp"
$begin=$task.IndexOf('s_task* taskListHead')
$end=$task.IndexOf('static p_workArea attachTask(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Task scheduler boundary changed'}
[IO.File]::WriteAllText("$scratch/scheduler.inc",$task.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc /DLAGI_NATIVE_TASK_HEAP_CLEANUP=1 "/I$scratch" "$PSScriptRoot/battle_task_heap_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Task heap test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Task heap test failed'}
