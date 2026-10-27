$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-format-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/rbg_tile_format_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'RBG format fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'RBG format fixture failed'}
$wrapper=Get-Content -Raw "$repo/shaders/vdp2_rbg0_tile16_8bit_f.cg"
if(!$wrapper.Contains('#define LAGI_RBG_TILE16_8BIT 1') -or !$wrapper.Contains('#include "vdp2_rbg0_f.cg"')){throw 'Variant diverged from generic shader'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$renderer.Contains('const bool decodedSource=frontend?g_frontendRbgDecodedReady:g_rbgDecodedPagesReady;') -or
   !$renderer.Contains('const bool decodedPages=decodedSource && prepareRbgPaletteLut(rawTextureData);') -or
   !$renderer.Contains('selectRbgTileProgram(((!frontend && g_sceneVdp2ByteTextureValid) || decodedPages) && lagi::rbgTile16EightBit(')){throw 'Format/source eligibility lost'}
foreach($variant in @('vdp2_rbg0_pages_f.cg','vdp2_rbg0_rows_f.cg')) {
    if(!(Get-Content -Raw "$repo/shaders/$variant").Contains('#define LAGI_RBG_PALETTE_LUT 1')) {
        throw "Decoded RBG variant lacks live palette LUT: $variant"
    }
}
$palette=Get-Content -Raw "$repo/src/platform/vita/neptune_rbg_palette_lut.inc"
if(!$palette.Contains('std::memcmp(g_rbgPaletteLutSource,cram,kCramBytes)') -or
   !$palette.Contains('vdp2Rgb555ToAbgr(readVdp2Be16(cram,i*2u))')) {
    throw 'RBG palette LUT lost live native CRAM conversion'
}
if(!$renderer.Contains('if (!spriteComposite && !clearNativeSceneColorBuffer(colorBuffer,')){throw 'Redundant final target clear restored'}
$binding=Get-Content -Raw "$repo/src/platform/vita/neptune_rbg_tile_program.inc"
if(!$binding.Contains('g_rbgGenericBindings[i]=*slots[i]') -or !$binding.Contains('use>=0?g_rbgBindings[use][i]:g_rbgGenericBindings[i]')){throw 'Variant uniform offsets not independently reflected/restored'}
