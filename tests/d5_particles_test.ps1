$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-d5-particles-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$header=Get-Content -Raw "$repo/include/lagi/lagi_d5_particles.h"
$source=Get-Content -Raw "$repo/src/integration/lagi_d5_particles.inc"
$body=($header+"`n"+$source) -replace '(?m)^#include[^\r\n]*',''
[IO.File]::WriteAllText((Join-Path $scratch 'd5_particle_fixture.inc'),$body)
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/d5_particles_test.cpp" "/I$scratch" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'D5 adapter fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'D5 adapter fixture failed'}
$s=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$s.IndexOf('static bool renderSceneVdp2BackgroundPass(bool frontend = false)')
$body=$s.Substring($start,$s.IndexOf('static void drawSceneVdp2Background()', $start)-$start)
if($body.IndexOf('drawPublishedVdp1Ui(Vdp1UiPass::FrontendProjected,&g_frontendSpriteSlot,true)') -gt
   $body.IndexOf('drawFrontendNbgNative()')){throw 'Particles must be below native keyboard'}
foreach($fragment in @('if(pass==Vdp1UiPass::FrontendProjected && !command.hasWorldDepth)continue;',
 'if(pass==Vdp1UiPass::FrontendOverlay && command.hasWorldDepth)continue;',
 'nativeCanvas ? 1.0f : (4.0f / 3.0f) / renderAspect;',
 'drawPublishedVdp1Ui(Vdp1UiPass::FrontendOverlay,&g_frontendSpriteSlot);')){
 if(!$s.Contains($fragment)){throw "Frontend particle layering contract lost: $fragment"}
}
Write-Output 'PASS native particle/keyboard/cursor layer and shared immutable sprite-slot source contracts'
