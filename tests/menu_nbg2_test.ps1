param([string]$ScratchRoot='..\menu-nbg2-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
foreach($entry in @(
 @('static std::uint32_t vdp2Rgb555ToAbgr(std::uint16_t color)','static std::uint32_t decodeVdp2Chsz1Pixel(','menu_nbg2_rgb_production.inc'),
 @('static std::uint32_t decodeMenuNbg2Pixel(','static void drawAzelMenuVdp2Gpu(','menu_nbg2_decode_production.inc'))) {
 $begin=$source.LastIndexOf($entry[0]);$end=$source.IndexOf($entry[1],$begin)
 if($begin -lt 0 -or $end -le $begin){throw 'Menu plane production boundary missing'}
 Set-Content -LiteralPath (Join-Path $scratch $entry[2]) -Value $source.Substring($begin,$end-$begin)
}
$begin=$source.LastIndexOf('static std::uint32_t decodeVdp2Chsz1Pixel(')
$end=$source.IndexOf('static std::uint32_t ',$begin+1)
if($begin -lt 0 -or $end -le $begin){throw 'Shared character decoder missing'}
Set-Content -LiteralPath (Join-Path $scratch 'menu_nbg2_character_production.inc') -Value $source.Substring($begin,$end-$begin)
foreach($guard in @('g_pendingMenuNbg2 = {};','g_menuNbg2 = g_pendingMenuNbg2;','g_menuNbg2 = {};',
 '!g_vdp2SaveLayout && lagi::menuNbg2Supported(g_menuNbg2)','lagi::menuNbg2Compose(g_menuNbg2,nbg0,nbg1,')) {
 if(!$source.Contains($guard)){throw "Menu NBG2 transport/composition guard missing: $guard"}
}
$exe=Join-Path $scratch 'menu_nbg2_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'menu_nbg2_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Menu NBG2 test compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Menu NBG2 test execution failed'}
