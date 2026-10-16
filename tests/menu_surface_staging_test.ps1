param([string]$ScratchRoot='../menu-surface-staging-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$pieces=@(
 @('static std::uint32_t vdp2Rgb555ToAbgr(std::uint16_t color)','static std::uint32_t decodeVdp2Chsz1Pixel(','rgb.inc'),
 @('static std::uint32_t decodeStatusMenuNbgPixel(','static void drawAzelMenuVdp2Gpu(','decode.inc'),
 @('    auto* gpuPixels = textOnly ?','    const auto menuSubmitStartUs =','compose.inc')
)
foreach($piece in $pieces) {
 $begin=$source.LastIndexOf($piece[0]);$end=$source.IndexOf($piece[1],$begin+1)
 if($begin -lt 0 -or $end -le $begin){throw "Production staging boundary missing: $($piece[0])"}
 [IO.File]::WriteAllText("$scratch/$($piece[2])",$source.Substring($begin,$end-$begin))
}
$begin=$source.LastIndexOf('static std::uint32_t decodeVdp2Chsz1Pixel(')
$end=$source.IndexOf('static std::uint32_t ',$begin+1)
if($begin -lt 0 -or $end -le $begin){throw 'Shared decoder boundary missing'}
[IO.File]::WriteAllText("$scratch/character.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/menu_surface_staging_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Surface staging compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Surface staging assertions failed'}
