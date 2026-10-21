$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$cmake=Get-Content -Raw "$repo/CMakeLists.txt"
if(!$cmake.Contains('option(LAGI_GPU_PASS_TIMING "Sample serialized native GPU pass timing (diagnostic stalls)" OFF)')){throw 'Diagnostic timing must default OFF'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$sample=$renderer.IndexOf('g_gpuPassSample=LAGI_GPU_PASS_TIMING && nativeSceneMode && !nativeMenuFrame')
$rbg=$renderer.IndexOf('const bool resolved = renderSceneVdp2BackgroundPass();',$sample)
$drain=$renderer.IndexOf('if(g_gpuPassSample) {',$rbg)
$begin=$renderer.IndexOf('const int beginSceneResult = sceGxmBeginScene(',$drain)
if($sample -lt 0 -or $rbg -lt $sample -or $drain -lt $rbg -or $begin -lt $drain -or !$renderer.Contains('++gpuPassFrame%120u==0')){throw 'RBG sample order or sampling cadence lost'}
$world=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
$end=$world.IndexOf('sceGxmEndScene(g_probeContext,nullptr,nullptr);')
$wait=$world.IndexOf('if(g_gpuPassSample) {',$end)
$restart=$world.IndexOf('if(sceGxmBeginScene(',$wait)
if($end -lt 0 -or $wait -lt $end -or $restart -lt $wait){throw 'World sample must drain after EndScene and before composition BeginScene'}
if(!$renderer.Contains('[GpuPassTiming] serialized=1') -or !$renderer.Contains('finalWait=%u')){throw 'Serialized samples must be explicitly labeled'}
Write-Output 'PASS opt-in sampled pass boundary timing contracts'
