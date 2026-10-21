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
if(!$renderer.Contains('selectRbgTileProgram(!frontend && g_sceneVdp2ByteTextureValid && lagi::rbgTile16EightBit(')){throw 'Native format selection lost'}
if(!$renderer.Contains('if (!spriteComposite && !clearNativeSceneColorBuffer(colorBuffer,')){throw 'Redundant final target clear restored'}
$binding=Get-Content -Raw "$repo/src/platform/vita/neptune_rbg_tile_program.inc"
if(!$binding.Contains('g_rbgGenericBindings[i]=*slots[i]') -or !$binding.Contains('use?g_rbgTileBindings[i]:g_rbgGenericBindings[i]')){throw 'Variant uniform offsets not independently reflected/restored'}
