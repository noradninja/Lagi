param([string]$ScratchRoot='../combat-profile-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_runtime.cpp"
$begin=$source.IndexOf('    const auto taskFrameStartUs =')
$end=$source.IndexOf('    // LAGI_TASK_FRAME_PROFILE_END',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native task profiler boundary changed'}
[IO.File]::WriteAllText("$scratch/task_profile.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "$PSScriptRoot/combat_profile_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Task profiler fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Task profiler fixture failed'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if($renderer -notmatch 'positionSync=%u positionSyncVerts=%u submissions=%u'){throw 'Combat geometry timings missing from ScenePerf'}
$start=$renderer.IndexOf('const std::uint64_t renderStartUs =')
$reset=$renderer.IndexOf('g_profilePositionSyncUs = 0u;', $start)
if($reset -lt $start -or $reset -gt $start+300){throw 'Position-sync metrics can leak from preceding frame'}
$copy=$renderer.IndexOf('const auto positionSyncStartUs =')
$finish=$renderer.IndexOf('sceKernelGetProcessTimeWide() - positionSyncStartUs)', $copy)
if($copy -lt 0 -or $finish -le $copy){throw 'Position sync loop is not timed'}
foreach($name in @('g_vdp1Vertices[i] = v;','g_vdp1LightingVertices[i] = v;','g_vdp1TextureVertices[i].z = v.z;','g_vdp1GouraudVertices[i].z = v.z;')){
 if(!$renderer.Substring($copy,$finish-$copy).Contains($name)){throw "Existing sync path changed without verification: $name"}
}
Write-Output 'PASS frame-reset and unchanged geometry-copy profiling boundary (not a performance result).'
