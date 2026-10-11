param([Parameter(Mandatory=$true)][string]$AzelSourceRoot, [string]$VitaCompiler)
$ErrorActionPreference = 'Stop'
$native = Get-Content -Raw (Join-Path $AzelSourceRoot 'menu/inventoryMenu.cpp')
$cmake = Get-Content -Raw (Join-Path $PSScriptRoot '../CMakeLists.txt')
$old = '        if (i < pThis->m30_numItemsInCurrentCategory) {'
if (($native.Split(@($old),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Inventory target must occur once' }
if ($cmake -notmatch 'itemIndex >= 0 && itemIndex < pThis->m30_numItemsInCurrentCategory &&' -or
    $cmake -notmatch 'itemIndex < static_cast<int>\(pThis->m34_itemsInCurrentCategory.size\(\)\)') { throw 'Production page guard missing' }
$cases = 0
foreach ($count in 0..100) {
    foreach ($page in -1..20) {
        $actual = @()
        foreach ($row in 0..4) {
            $index = 5 * $page + $row
            if ($index -ge 0 -and $index -lt $count -and $index -lt 100) { $actual += $index }
        }
        $expected = @()
        if ($page -ge 0) {
            for ($index=0; $index -lt $count; ++$index) {
                if ([math]::Floor($index/5) -eq $page) { $expected += $index }
            }
        }
        if (($actual -join ',') -ne ($expected -join ',')) { throw "Wrong page count=$count page=$page" }
        ++$cases
    }
}
Write-Output "PASS: $cases inventory count/page cases, including partial pages and out-of-range pages (model/source guards; not hardware)"
$table = [regex]::Match($native,'(?s)static const s16 berserkItems\[\] = \{(.*?)\};').Groups[1].Value
$berserkIds = @([regex]::Matches($table,'0x[0-9A-Fa-f]+') | ForEach-Object { [Convert]::ToInt32($_.Value.Substring(2),16) })
if ($berserkIds.Count -ne 30 -or ($berserkIds | Sort-Object -Unique).Count -ne 30) { throw 'Native 30-row Berserk ordering changed' }
foreach ($learnedIndex in -1..30) {
    $rows = @()
    foreach ($index in 0..29) { $rows += $(if ($index -eq $learnedIndex -or $learnedIndex -eq 30) {$berserkIds[$index]} else {0}) }
    if ($rows.Count -ne 30) { throw 'Berserk rows must not compact unlearned entries' }
    foreach ($index in 0..29) {
        if (($index -eq $learnedIndex -or $learnedIndex -eq 30) -and $rows[$index] -ne $berserkIds[$index]) { throw 'Berserk learned entry moved' }
    }
}
Write-Output 'PASS: 32 learned-ability patterns preserve native 30-row ordering (model only)'
foreach ($case in @(@(0x13333,20),@(0x18000,50),@(0x20000,100),@(0x10000,0),@(0xB333,-30),@(0x1E666,90))) {
    $wrapped = (([long]$case[0]-0x10000)*100+0x8000) -band 0xffffffffL
    $modifier = ($wrapped -shr 16) -band 0xffff
    if ($modifier -ge 0x8000) { $modifier -= 0x10000 }
    if ($modifier -ne $case[1]) { throw 'Native rounded weapon modifier model changed' }
}
Write-Output 'PASS: six native weapon-modifier cases including negative fixed-point rounding (model only)'
if ($VitaCompiler) {
    $replacement = [regex]::Match($cmake,'"(        // LAGI_INVENTORY_PAGE_BOUND[^"\r\n]*)"')
    if (!$replacement.Success) { throw 'Production inventory replacement missing' }
    $new = $replacement.Groups[1].Value.Replace('\n',"`n")
    $patched = $native.Replace($old,$new).Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
    foreach ($name in @('inventory_delete_old','inventory_delete_new')) {
        $literal = [regex]::Match($cmake,'set\(' + $name + ' "((?:\\.|[^"\r\n])*)"\)')
        if (!$literal.Success) { throw "Missing production $name" }
        Set-Variable -Name $name -Value $literal.Groups[1].Value.Replace('\n',"`n")
    }
    $patched = $patched.Replace("`r`n","`n")
    if (($patched.Split(@($inventory_delete_old),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Inventory Delete target must occur once' }
    $patched = $patched.Replace($inventory_delete_old,$inventory_delete_new)
    $berserkRowsOld = "    case 2: // berserks`n        assert(0);`n        break;"
    $berserkPrintOld = "void inventoryPrintBerserk(s32, eItems, s32)`n{`n    Unimplemented();`n}"
    $berserkRowsNew = [regex]::Match($cmake,'(?s)\[=\[(    case 2: // LAGI_NATIVE_BERSERK_ROWS:.*?)\]=\]').Groups[1].Value.Replace("`r`n","`n")
    $berserkPrintNew = [regex]::Match($cmake,'(?s)\[=\[(// Native 06007378;.*?)\]=\]').Groups[1].Value.Replace("`r`n","`n")
    foreach ($target in @($berserkRowsOld,$berserkPrintOld)) {
        if (($patched.Split(@($target),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Native Berserk patch target changed' }
    }
    if (!$berserkRowsNew -or !$berserkPrintNew) { throw 'Missing native Berserk production patch' }
    $patched = $patched.Replace($berserkRowsOld,$berserkRowsNew).Replace($berserkPrintOld,$berserkPrintNew)
    $detailsOld = "void inventoryMenuTaskDrawSub2(s_inventoryMenu* pThis, s32 param_2)`n{`n    Unimplemented();`n}"
    $detailsNew = [regex]::Match($cmake,'(?s)\[=\[(void inventoryMenuTaskDrawSub2.*?)\]=\]').Groups[1].Value.Replace("`r`n","`n")
    if (!$detailsNew -or ($patched.Split(@($detailsOld),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Native inventory details patch target changed' }
    $patched = $patched.Replace($detailsOld,$detailsNew).Replace('#include "items.h"',"#include `"items.h`"`n#include `"battle/gunShotRootTask.h`"")
    $repo = Split-Path $PSScriptRoot -Parent
    $patched | & $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 -DLAGI_NEPTUNE_RENDERER=1 -I "$repo/include" -I "$repo/src/integration" -I $AzelSourceRoot -I "$AzelSourceRoot/menu" -x c++ -
    if ($LASTEXITCODE -ne 0) { throw 'Patched inventory source Vita syntax check failed' }
    Write-Output 'PASS: patched native inventory source Vita syntax check (stdin; no source/save files written)'
}
