param([string]$Compiler=$env:PSP2CGC)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-ratio-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$shader=Get-Content -Raw "$repo/shaders/rbg_ratio_composite_f.cg"
$hostSource=$shader -replace ':TEX(?:COORD|UNIT)\d+',''
[IO.File]::WriteAllText("$scratch/rbg_ratio_composite_host.inc",$hostSource)
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/rbg_ratio_composite_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'RBG ratio fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'RBG ratio fixture failed'}
& $Compiler -profile sce_fp_psp2 "$repo/shaders/rbg_ratio_composite_f.cg" -Wperf -fastprecision -O3 -xmlsdb "$scratch/shader.xml" -o "$scratch/shader.gxp"
if($LASTEXITCODE){throw 'RBG ratio shader compile failed'}
[xml]$xml=Get-Content -Raw "$scratch/shader.xml"
foreach($binding in @(@('layer',0),@('background',1),@('decisions',2),@('nbg0',3))){
    $resource=@($xml.sdb.resourceInfo.sampler | Where-Object {$_.symbolName -eq $binding[0]})
    if($resource.Count -ne 1 -or $resource[0].textureUnit -ne [string]$binding[1]){throw "Wrong sampler $($binding[0])"}
}
foreach($name in 'geometry','backColor','rbgState'){
    $resource=@($xml.sdb.resourceInfo.uniform | Where-Object {$_.symbolName -eq $name})
    if($resource.Count -ne 1 -or $resource[0].leafFormat -ne 'fp32:4'){throw "Wrong uniform $name"}
}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$service=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
$bridge=Get-Content -Raw "$repo/src/integration/lagi_scene_bridge.cpp"
foreach($capture in 'sprite.specialPriority=regs->mEA_SFPRMD;','sprite.specialColor=regs->mEE_SFCCMD;'){
    if(!$bridge.Contains($capture)){throw 'Special-function support gate is not frame matched'}
}
$start=$renderer.IndexOf('static void applyPendingRendererInvalidation()')
$end=$renderer.IndexOf('const std::uint32_t flags =',$start)
$formatBoundary=$renderer.Substring($start,$end-$start)
if(!$formatBoundary.Contains('decodedType0!=type0') -or !$formatBoundary.Contains('++g_nativeTextureGeneration;') -or
    !$formatBoundary.Contains('entry.contentsDirty=true;')){throw 'Texture metadata format boundary does not invalidate both model and UI caches'}
if(!$renderer.Contains('if (!spriteComposite && g_sceneNbg0.behindWorld)') -or
    !$renderer.Contains('if (!spriteComposite && !g_sceneNbg0.behindWorld)')){throw 'NBG0 would be drawn twice or inside VDP1 layer'}
if(!$renderer.Contains('const bool completedForeground=priorityUi || (spriteComposite && g_rbgRatioCompositeFrame);') -or
    !$renderer.Contains('if(!completedForeground)drawPublishedVdp1Ui(')){throw 'WORLD foreground is not part of completed VDP1 image'}
if(!$service.Contains('sceGxmSetFragmentTexture(g_probeContext,3,(g_spriteColor.bgon&1)?&g_sceneNbg0Texture:')){throw 'NBG0 raw second source not bound'}
Write-Output 'PASS Vita shader compilation/reflection; hardware composition and performance pending'
