$ErrorActionPreference = 'Stop'
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/platform/vita/neptune_renderer.cpp')
if ($source -notmatch '(?s)drawAzelMenuVdp2Gpu\(\);\s*drawPublishedVdp1Ui\(.*?drawAzelMenuVdp2Gpu\(true\);') {
    throw 'Native menu background / sprite / glyph ordering changed'
}
if ($source -notmatch 'textOnly \? g_vdp2TextLayerPixels : g_vdp2MenuBackgroundPixels' -or
    $source -notmatch 'freeSimpleMappedProbe\(g_vdp2MenuBackgroundUid, menuBackgroundPtr\)') {
    throw 'Menu background must own separately released GPU storage'
}
Write-Output 'PASS: menu composition source guards (not GPU or device execution)'
