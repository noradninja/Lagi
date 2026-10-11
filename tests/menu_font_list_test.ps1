param([Parameter(Mandatory=$true)][string]$AzelSourceRoot, [string]$VitaCompiler)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$cmake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$native = (Get-Content -Raw (Join-Path $AzelSourceRoot 'VDP2.cpp')).Replace("`r`n","`n")
function Get-Literal($name) {
    $match = [regex]::Match($cmake, 'set\(' + $name + ' "((?:\\.|[^"\r\n])*)"\)')
    if (!$match.Success) { throw "Missing production literal $name" }
    return $match.Groups[1].Value.Replace('\n',"`n").Replace('\"','"')
}
$old = Get-Literal 'font_link_old'
$new = Get-Literal 'font_link_new'
$menuNative = Get-Content -Raw (Join-Path $AzelSourceRoot 'mainMenuDebugTasks.cpp')
$menuDeleteOld = Get-Literal 'menu_font_delete_old'
$menuDeleteNew = Get-Literal 'menu_font_delete_new'
if (($menuNative.Split(@($menuDeleteOld),[StringSplitOptions]::None).Length-1) -ne 1 -or $menuDeleteNew -notmatch 'lagiAzelUnloadFontByIndex\(graphicEngineStatus.m40AC.fontIndex\)') { throw 'Outer-menu font cleanup target changed' }
Write-Output 'PASS: outer-menu native Delete patch matches once (source guard)'
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
$rectOld = Get-Literal 'font_rect_old'
$rectNew = Get-Literal 'font_rect_new'
$bottomLeftOld = Get-Literal 'font_bottom_left_old'
$bottomLeftNew = Get-Literal 'font_bottom_left_new'
if (($native.Split(@($bottomLeftOld),[StringSplitOptions]::None).Length-1) -ne 1 -or $bottomLeftNew -notmatch 'm4_cursorY \+ 1\] = characterSprite \+ 2') { throw 'Native bottom-left glyph map target changed' }
foreach ($x in 0..62) {
    foreach ($y in 0..62) {
        $indices = @(($x*64+$y),(($x+1)*64+$y),($x*64+$y+1),(($x+1)*64+$y+1))
        if (($indices | Sort-Object -Unique).Count -ne 4 -or ($indices | Measure-Object -Maximum).Maximum -ge 4096) { throw '2x2 glyph map cells overlap or exceed map' }
    }
}
Write-Output 'PASS: bottom-left native patch and 3969 distinct 2x2 glyph-map layouts (model only)'
if (($native.Split(@($rectOld),[StringSplitOptions]::None).Length-1) -ne 1 -or $rectNew -notmatch '\(r5 \+ row\) \* 64 \+ r4' -or $rectNew -notmatch 'r6 \* 2') { throw 'Native rectangle stride/width changed' }
# The production caller clears 44 by 32 cells; preserve the other 20 cells
# of each row and all text-map rows outside the rectangle.
$cells = New-Object byte[] (64*64*2)
[Array]::Fill[byte]($cells, 0xA5)
foreach ($row in 0..31) { [Array]::Clear($cells,$row*128,44*2) }
for ($row=0;$row -lt 64;++$row) {
    for ($column=0;$column -lt 64;++$column) {
        $expected = if ($row -lt 32 -and $column -lt 44) {0} else {0xA5}
        if ($cells[($row*64+$column)*2] -ne $expected -or $cells[($row*64+$column)*2+1] -ne $expected) { throw 'Rectangle model changed adjacent cells' }
    }
}
Write-Output 'PASS: native rectangle source guard and 4096-cell preservation model (not runtime execution)'
$numberCases = 0
foreach ($value in @(0,1,9,10,99,100,255,999,1000,-1,-99,[int]::MaxValue,[int]::MinValue)) {
    foreach ($width in @(0,1,2,3,10,11,12)) {
        $digits = New-Object int[] 11
        $first = 11
        $magnitude = [long]$value
        if ($magnitude -lt 0) { $magnitude = -$magnitude }
        do {
            --$first
            $digits[$first] = 0x8002 + ($magnitude % 10)
            $magnitude = [long][math]::Floor($magnitude/10)
        } while ($magnitude)
        if ($value -lt 0) { --$first; $digits[$first] = 0x8041 }
        $effectiveWidth = [math]::Min($width,11)
        while ($effectiveWidth -gt 0 -and 11-$first -lt $effectiveWidth) { --$first; $digits[$first] = 0 }
        $text = ''
        foreach ($glyph in $digits[$first..10]) {
            $text += $(if ($glyph -eq 0) {' '} elseif ($glyph -eq 0x8041) {'-'} else {[string]($glyph-0x8002)})
        }
        $expected = ([string]$value).PadLeft([math]::Max($effectiveWidth,([string]$value).Length))
        if ($text -ne $expected) { throw "Number format model failed value=$value width=$width" }
        ++$numberCases
    }
}
Write-Output "PASS: $numberCases encoded-number format models including signed limits; not runtime execution"
if ($VitaCompiler) {
    $patched = $native.Replace($old,$new).Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
    $patched = $patched.Replace($rectOld,$rectNew)
    $patched = $patched.Replace($bottomLeftOld,$bottomLeftNew)
    $numberOld = "void printVdp2Number3(s_stringStatusQuery* vars, int value, int length)`n{`n    Unimplemented();`n}"
    $numberNew = [regex]::Match($cmake,'(?s)\[=\[(int loadExtendedCharacter\(u16 character\);.*?)\]=\]').Groups[1].Value.Replace("`r`n","`n")
    if (!$numberNew -or ($patched.Split(@($numberOld),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Native number printer target changed' }
    $patched = $patched.Replace($numberOld,$numberNew)
    $unload = [regex]::Match($cmake,'(?s)\[=\[(// LAGI_NATIVE_FONT_UNLOAD:.*?)\]=\]').Groups[1].Value.Replace("`r`n","`n")
    if (!$unload -or ($patched.Split(@('s32 resetVdp2StringsSub1(u16* pData)'),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Font unload insertion target changed' }
    $patched = $patched.Replace('s32 resetVdp2StringsSub1(u16* pData)',$unload)
    $patched | & $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 -DLAGI_NEPTUNE_RENDERER=1 -I "$repo/include" -I "$repo/src/integration" -I $AzelSourceRoot -I "$AzelSourceRoot/kernel" -x c++ -
    if ($LASTEXITCODE -ne 0) { throw 'Patched VDP2 Vita syntax check failed' }
    Write-Output 'PASS: patched native VDP2 Vita syntax check; no upstream source written'
}
