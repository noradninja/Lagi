param([Parameter(Mandatory=$true)][string]$AzelSourceRoot)
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
