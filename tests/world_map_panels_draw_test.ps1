$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-panels-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_map_panels.cpp"
[IO.File]::WriteAllText("$scratch/production.inc",[regex]::Replace($source,'(?m)^#include[^\r\n]*',''))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_panels_draw_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Panel draw fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Panel draw fixture failed'}
$runtime=Get-Content -Raw "$repo/src/integration/lagi_runtime.cpp"
if(!$runtime.Contains('leftEdgeCommandOwned.fill(false);') -or
    !$runtime.Contains('ui.leftEdgeOrigin=leftEdgeCommandOrigin[nativeIndex];')){throw 'Per-frame producer ownership/anchor publication missing'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$renderer.Contains('if(command.leftEdgeAnchored && !nativeCanvas && commandType==0u)') -or
    !$renderer.Contains('lagi::uiLeftEdgeShift(command.leftEdgeOrigin,saturnAspectCorrection)')){throw 'Presentation-only left anchoring missing'}
