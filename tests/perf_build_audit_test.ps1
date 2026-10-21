param([string]$ScratchRoot='../perf-build-audit-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
Copy-Item -LiteralPath "$PSScriptRoot/perf_build_audit_fixture.ninja" -Destination "$scratch/build.ninja"
$rows=@(& "$repo/tools/audit_vita_perf_build.ps1" -Build $scratch)
$neptune=$rows | Where-Object Source -eq Neptune
$math=$rows | Where-Object Source -eq Native3D
$animation=$rows | Where-Object Source -eq Animation
$queen=$rows | Where-Object Source -eq BroodQueen
$missing=$rows | Where-Object Source -eq Urchin
if(!$neptune.Found -or $neptune.Optimization -ne '-O2' -or $neptune.NDebug -or $neptune.ExplicitFastMath){throw 'Renderer flag attribution failed'}
if($math.Optimization -ne '-O0' -or !$math.NDebug -or $math.ExplicitFastMath){throw 'Last flag precedence or NDEBUG failed'}
if($animation.Optimization -ne 'compiler default (no -O flag)'){throw 'Missing optimization incorrectly inferred'}
if($queen.Optimization -ne '-O3' -or !$queen.ExplicitFastMath){throw 'Queen options mixed with another source'}
if($missing.Found){throw 'Missing source reported as built'}
Write-Output 'PASS per-source optimization audit, last-option precedence, assertion/fast-math flags and missing sources.'
