param([Parameter(Mandatory=$true)][string]$AzelSourceRoot, [string]$VitaCompiler)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$cmake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$native = (Get-Content -Raw (Join-Path $AzelSourceRoot 'VDP2.cpp')).Replace("`r`n","`n")
function Get-Literal($name) {
    $match = [regex]::Match($cmake, 'set\(' + $name + ' "((?:\\.|[^"\r\n])*)"\)')
    if (!$match.Success) { throw "Missing production literal $name" }
    return $match.Groups[1].Value.Replace('\n',"`n")
}
$old = Get-Literal 'font_link_old'
$new = Get-Literal 'font_link_new'
if (($native.Split(@($old),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Font link target must occur once' }
if ($new -notmatch 'pOld->pNext = pNew;' -or $new -notmatch 'pNew->m15 = 0;') { throw 'Native link/ownership initialization missing' }
# Linked-list model of the production tail append and active-font lookup.
$base = @{index=0;next=$null;previous=$null}
foreach ($index in 1..32) {
    $tail = $base
    while ($null -ne $tail.next) { $tail = $tail.next }
    $node = @{index=($tail.index+1);next=$null;previous=$tail;flags=0}
    $tail.next = $node
    if ($node.index -ne $index -or $node.previous.index -ne ($index-1)) { throw 'Append identity failed' }
}
foreach ($index in 0..32) {
    $node = $base
    while ($null -ne $node -and $node.index -ne $index) { $node = $node.next }
    if ($null -eq $node) { throw "Font $index cannot be restored" }
}
Write-Output 'PASS: production font patch matches once; 33-node append/lookup model (not runtime execution)'
if ($VitaCompiler) {
    $patched = $native.Replace($old,$new).Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
    $unload = [regex]::Match($cmake,'(?s)\[=\[(// LAGI_NATIVE_FONT_UNLOAD:.*?)\]=\]').Groups[1].Value.Replace("`r`n","`n")
    if (!$unload -or ($patched.Split(@('s32 resetVdp2StringsSub1(u16* pData)'),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Font unload insertion target changed' }
    $patched = $patched.Replace('s32 resetVdp2StringsSub1(u16* pData)',$unload)
    $patched | & $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 -DLAGI_NEPTUNE_RENDERER=1 -I "$repo/include" -I "$repo/src/integration" -I $AzelSourceRoot -I "$AzelSourceRoot/kernel" -x c++ -
    if ($LASTEXITCODE -ne 0) { throw 'Patched VDP2 Vita syntax check failed' }
    Write-Output 'PASS: patched native VDP2 Vita syntax check; no upstream source written'
}
