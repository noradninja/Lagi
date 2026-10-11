param([Parameter(Mandatory=$true)][string]$AzelSourceRoot,[Parameter(Mandatory=$true)][string]$VitaCompiler)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$cmake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$start = $cmake.IndexOf('elseif(relative_path STREQUAL "kernel/menuCursor.cpp")')
$end = $cmake.IndexOf('   string(REGEX MATCHALL',$start)
$block = $cmake.Substring($start,$end-$start)
$native = (Get-Content -Raw (Join-Path $AzelSourceRoot 'kernel/menuCursor.cpp')).Replace("`r`n","`n")
$patches = [regex]::Matches($block,'(?s)string\(REPLACE\s+"((?:\\.|[^"\\])*)"\s+"((?:\\.|[^"\\])*)"\s+source_text')
if ($patches.Count -ne 4) { throw 'Expected four native cursor patches' }
foreach ($patch in $patches) {
    $old = [regex]::Unescape($patch.Groups[1].Value)
    $new = [regex]::Unescape($patch.Groups[2].Value)
    if (($native.Split(@($old),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Cursor patch target must occur once' }
    $native = $native.Replace($old,$new)
}
$native = $native.Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
$native | & $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 -DLAGI_NEPTUNE_RENDERER=1 -I "$repo/include" -I "$repo/src/integration" -I $AzelSourceRoot -I "$AzelSourceRoot/kernel" -x c++ -
if ($LASTEXITCODE -ne 0) { throw 'Patched native cursor Vita syntax check failed' }
Write-Output 'PASS: four exact cursor source patches and Vita syntax compilation; no runtime animation claim'
