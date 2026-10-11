param([Parameter(Mandatory=$true)][string]$AzelSourceRoot,[Parameter(Mandatory=$true)][string]$VitaCompiler)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$source = Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_books_menu.cpp')
if ($source -notmatch '0x20DDC8' -or $source -notmatch 'getBit\(0xF3 \+ item\)' -or $source -notmatch 'ownedIndices\[result.count\+\+\]' -or $source -notmatch 'result.selected = position') { throw 'Native book-list production guards changed' }
$cases = 0
foreach ($pattern in -1..22) {
    $owned = @()
    foreach ($index in 0..21) { if ($index -eq $pattern -or $pattern -eq 22) { $owned += $index } }
    foreach ($requested in -1..22) {
        $selected = 0
        foreach ($position in 0..($owned.Count-1)) {
            if ($owned.Count -gt 0 -and $owned[$position] -eq $requested) { $selected=$position; break }
        }
        $expected = [Array]::IndexOf($owned,$requested)
        if ($expected -lt 0) { $expected=0 }
        if ($selected -ne $expected -or $owned.Count -gt 22) { throw 'Native owned-index selection model failed' }
        ++$cases
    }
}
Write-Output "PASS: $cases Books ownership/selection models; not native task or hardware execution"
& $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 -DLAGI_NEPTUNE_RENDERER=1 -I "$repo/include" -I "$repo/src/integration" -I $AzelSourceRoot (Join-Path $repo 'src/integration/lagi_azel_books_menu.cpp')
if ($LASTEXITCODE -ne 0) { throw 'Books helper Vita syntax check failed' }
Write-Output 'PASS: Books lifecycle/selection helper Vita syntax check'
& $VitaCompiler -std=c++20 -fsyntax-only -I "$repo/include" (Join-Path $repo 'src/integration/lagi_azel_books_resource.cpp')
if ($LASTEXITCODE -ne 0) { throw 'Books resource reader Vita syntax check failed' }
Write-Output 'PASS: portable Books resource reader Vita syntax check'
