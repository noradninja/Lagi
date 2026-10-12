param([string]$ScratchRoot='..\pickup-composition-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$begin=$source.IndexOf('static bool vdp1UiPassAccepts(')
$end=$source.IndexOf('static bool isVdp1BackgroundCommand(', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production pass function missing'}
Set-Content -LiteralPath (Join-Path $scratch 'pickup_pass_production.inc') -Value $source.Substring($begin,$end-$begin)
$begin=$source.IndexOf('// LAGI_PICKUP_EFFECTS_BELOW_UI:')
$end=$source.IndexOf('// All native scenes share', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Gameplay composition boundary missing'}
$branch=$source.Substring($begin,$end-$begin)
$effects=$branch.IndexOf('Vdp1UiPass::WorldEffects, &vdp1UiSpriteSlot')
$tiles=$branch.IndexOf('drawAzelVdp2Nbg1Gpu();')
$text=$branch.IndexOf('drawAzelVdp2TextLayerGpu();')
$overlay=$branch.IndexOf('Vdp1UiPass::Foreground, &vdp1UiSpriteSlot')
if($effects -lt 0 -or $tiles -le $effects -or $text -le $tiles -or $overlay -le $text){throw 'Effects/pickup tile/text/overlay order incorrect'}
$exe=Join-Path $scratch 'pickup_composition_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'pickup_composition_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Composition test compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Composition test execution failed'}
