param([string]$ScratchRoot='..\menu-canvas-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$begin=$source.IndexOf('static float menuCanvasHalfWidth(')
$end=$source.IndexOf('static void drawMenuCanvasMask(', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Canvas helper missing'}
Set-Content -LiteralPath (Join-Path $scratch 'menu_canvas_production.inc') -Value $source.Substring($begin,$end-$begin)
if(!$source.Contains('drawAzelMenuVdp2Gpu(true);' + "`n" + '            drawMenuCanvasMask();')) {
 if($source -notmatch 'drawAzelMenuVdp2Gpu\(true\);\s+drawMenuCanvasMask\(\);'){throw 'Menu mask must follow final glyph plane'}
}
if(!$source.Contains('(g_vdp2MenuId==0u && !g_vdp2SaveLayout)') -or
 !$source.Contains('freeSimpleMappedProbe(g_menuCanvasMaskVertexUid') -or
 !$source.Contains('freeSimpleMappedProbe(g_menuCanvasMaskIndexUid')){throw 'Menu gate or resource cleanup missing'}
$exe=Join-Path $scratch 'menu_canvas_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'menu_canvas_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Canvas test compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Canvas test execution failed'}
