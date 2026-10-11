param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib', [string]$VitaCompiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe')
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$cmake=Get-Content -Raw (Join-Path $root 'CMakeLists.txt')
$source=(Get-Content -Raw (Join-Path $AzelSourceRoot 'field/fieldRadar.cpp')).Replace("`r`n","`n")
$pairs=@(
    @('mC_X = pTypedWorkArea->m8 + 0x30','mC_X = pTypedWorkArea->m8 + 0x60'),
    @('m34 = 0x18','m34 = 0x0A'),
    @('m28 = -0x1D','m28 = 0x1D')
)
foreach($pair in $pairs) {
    if (!$cmake.Contains(($pair -join '|'))) {throw "Missing generated adapter: $($pair[0])"}
    if ([regex]::Matches($source,[regex]::Escape($pair[0])).Count -ne 1) {throw "Initializer anchor not unique: $($pair[0])"}
    $source=$source.Replace($pair[0],$pair[1])
}
$source=$source.Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
$source | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/field" -
if ($LASTEXITCODE -ne 0) {throw 'Radar initializer syntax failed'}
# Cardinal directions: native outer/inner radii produce a 14-unit segment,
# with center 29 units right of the frame anchor, not 29 units left.
foreach($direction in @(@(0,65536),@(65536,0),@(0,-65536),@(-65536,0))) {
    $dx=14*$direction[0]/65536
    $dy=14*$direction[1]/65536
    if (($dx*$dx+$dy*$dy) -ne 196) {throw 'Degenerate native direction segment'}
}
Write-Output 'PASS: native radar initializer anchors, cardinal geometry and Vita syntax; no hardware validation'
