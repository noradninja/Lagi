param([string]$ScratchRoot='../battle-allocator-tables-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$service=Get-Content -Raw "$repo/src/integration/lagi_allocator_tables_vita.cpp"
$begin=$service.IndexOf('namespace {')
if($begin -lt 0){throw 'Allocator table service boundary changed'}
[IO.File]::WriteAllText("$scratch/service.inc",$service.Substring($begin))
$native=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/mainMenuDebugTasks.cpp"
foreach($part in @(@('void initDramAllocator(','// Replace Saturn DRAM allocator','push.inc'),@('void freeRamResources(','// 06031742','pop.inc'),@('void resetTempAllocators()','void loadRamResource(','reset.inc'))){
 $begin=$native.IndexOf($part[0]);$end=$native.IndexOf($part[1],$begin)
 if($begin -lt 0 -or $end -le $begin){throw "Generated allocator boundary changed: $($part[0])"}
 [IO.File]::WriteAllText((Join-Path $scratch $part[2]),$native.Substring($begin,$end-$begin))
}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/battle_allocator_tables_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Allocator table test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Allocator table test failed'}
