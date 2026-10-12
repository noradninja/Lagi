param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib', [string]$VitaCompiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe')
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$cmake=(Get-Content -Raw (Join-Path $root 'CMakeLists.txt')).Replace("`r`n","`n")
$source=(Get-Content -Raw (Join-Path $AzelSourceRoot 'VDP2.cpp')).Replace("`r`n","`n")
$old=[regex]::Match($cmake,'set\(glyph_clear_old \[=\[([\s\S]*?)\]=\]\)').Groups[1].Value
$new=[regex]::Match($cmake,'set\(glyph_clear_new \[=\[([\s\S]*?)\]=\]\)').Groups[1].Value
$read='    u16 var0 = characterMap2[(y << 6) + x];'
foreach($anchor in @($old,$read)) {
    if (!$anchor -or [regex]::Matches($source,[regex]::Escape($anchor)).Count -ne 1) {throw 'Glyph clear anchor not unique'}
}
$source=$source.Replace($old,$new).Replace($read,'    u16 var0 = characterMap2[x * 64 + y];')
$source=$source.Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
$source | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" -
if ($LASTEXITCODE -ne 0) {throw 'Glyph clear Vita syntax failed'}
# Native low-two-bit cell encoding recovers the same origin from all four cells.
$cases=0
for($x=0;$x -lt 63;$x++) {for($y=0;$y -lt 63;$y++) {for($cell=0;$cell -lt 4;$cell++) {
    $qx=$x+($cell -band 1); $qy=$y+(($cell -band 2) -shr 1)
    $ox=$qx-($cell -band 1); $oy=$qy-(($cell -band 2) -shr 1)
    if ($ox -ne $x -or $oy -ne $y) {throw 'Origin recovery failed'}
    $ownership=@(($ox*64+$oy),(($ox+1)*64+$oy),($ox*64+$oy+1),(($ox+1)*64+$oy+1))
    if (($ownership | Sort-Object -Unique).Count -ne 4 -or ($ownership | Measure-Object -Maximum).Maximum -ge 4096) {throw 'Ownership geometry failed'}
    $cases++
}}}
Write-Output "PASS: patched glyph clear Vita syntax and $cases origin/index model cases; not hardware validation"
