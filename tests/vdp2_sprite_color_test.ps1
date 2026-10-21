$ErrorActionPreference='Stop'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-vdp2-sprite-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/vdp2_sprite_color_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'VDP2 sprite fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'VDP2 sprite fixture failed'}
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$begin=$renderer.IndexOf('Vdp1UiPass::WorldEffects, &vdp1UiSpriteSlot);', $renderer.IndexOf('const std::uint64_t composeStartUs'))
$finish=$renderer.IndexOf('finishSpriteComposite(renderTarget,colorSurface,syncObject)',$begin)
$text=$renderer.IndexOf('drawAzelVdp2TextLayerGpu();',$begin)
if($begin -lt 0 -or $finish -lt $begin -or $text -lt $finish){throw 'Sprite composition is not between completed world effects and NBG text'}
if(!$renderer.Contains('spriteComposite ? &g_spriteLayerSurface : colorSurface')){throw 'No separate VDP1 color target'}
