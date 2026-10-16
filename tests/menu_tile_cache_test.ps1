param([string]$ScratchRoot='../menu-tile-cache-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
foreach($entry in @(
 @('static std::uint32_t vdp2Rgb555ToAbgr(std::uint16_t color)','static std::uint32_t decodeVdp2Chsz1Pixel(','menu_tile_rgb.inc'),
 @('static std::uint32_t decodeSaveMenuCharacter(std::uint16_t patternName,int x,int y);','static void overlayAzelTextPlane(','menu_tile_cache_production.inc'))){
 $start=$source.LastIndexOf($entry[0]);$end=$source.IndexOf($entry[1],$start)
 if($start -lt 0 -or $end -lt 0){throw 'Menu tile production boundary missing'}
 Set-Content "$scratch/$($entry[2])" $source.Substring($start,$end-$start)
}
$start=$source.LastIndexOf('static std::uint32_t decodeVdp2Chsz1Pixel(')
$end=$source.IndexOf('static std::uint32_t ',$start+1)
if($start -lt 0 -or $end -lt 0){throw 'Native character decoder missing'}
Set-Content "$scratch/menu_tile_character.inc" $source.Substring($start,$end-$start)
if(!$source.Contains('if(!textOnly)g_menuTileCache.beginFrame();')){throw 'Immutable-frame cache boundary missing'}
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/menu_tile_cache_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Menu tile cache compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Menu tile cache test failed'}
