param([string]$Compiler=$env:PSP2CGC)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-ui-priority-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/sprite_ui_priority_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Sprite UI priority fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Sprite UI priority fixture failed'}
foreach($dither in 0,1) { foreach($lookup in 0,1) {
    $xmlPath=Join-Path $scratch "priority-$dither-$lookup.xml"
    & $Compiler -profile sce_fp_psp2 "$repo/shaders/vdp2_ui_priority_f.cg" "-DLAGI_UI_DITHER=$dither" "-DLAGI_DITHER_LOOKUP=$lookup" -Wperf -fastprecision -O3 -xmlsdb $xmlPath -o (Join-Path $scratch "priority-$dither-$lookup.gxp")
    if($LASTEXITCODE -ne 0){throw 'Priority shader compile failed'}
    [xml]$xml=Get-Content -Raw $xmlPath
    $resources=$xml.sdb.resourceInfo
    foreach($binding in @(@('tex',0),@('spriteLayer',2),@('spriteDecisions',3))) {
        $resource=@($resources.sampler | Where-Object {$_.symbolName -eq $binding[0]})
        if($resource.Count -ne 1 -or $resource[0].textureUnit -ne [string]$binding[1]){throw "Missing sampler $($binding[0])"}
    }
    $geometry=@($resources.uniform | Where-Object {$_.symbolName -eq 'priorityGeometry'})
    if($geometry.Count -ne 1 -or $geometry[0].leafFormat -ne 'fp32:4'){throw 'Priority uniform reflection mismatch'}
    $size=@($resources.uniform | Where-Object {$_.symbolName -eq 'textureSize'})
    if($size.Count -ne $dither){throw 'Sampling variant reflection mismatch'}
}}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$renderer.IndexOf('const bool priorityUi=spriteComposite && g_spriteUiPriorityFrame;')
$foreground=$renderer.IndexOf('Vdp1UiPass::Foreground, &vdp1UiSpriteSlot);',$start)
$resolve=$renderer.IndexOf('finishSpriteComposite(renderTarget,colorSurface,syncObject)',$start)
$panel=$renderer.IndexOf('drawAzelVdp2Nbg1Gpu();',$start)
if($start -lt 0 -or $foreground -lt $start -or $resolve -lt $foreground -or $panel -lt $resolve){throw 'Completed VDP1 priority resolve order lost'}
$bridge=Get-Content -Raw "$repo/src/integration/lagi_scene_bridge.cpp"
if(!$bridge.Contains('sprite.prina=regs->mF8_PRINA; sprite.prinb=regs->mFA_PRINB;')){throw 'UI priorities not frame-matched to native registers'}
$service=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
$prepare=$service.Substring($service.IndexOf('static bool prepareSpriteUiPriority()'),$service.IndexOf('static void releaseSpriteComposite()')-$service.IndexOf('static bool prepareSpriteUiPriority()'))
if($prepare -match 'probeGpuAlloc|probeCdramAlloc|sceGxmBeginScene|sceGxmFinish|sceGxmDraw'){throw 'Priority mask added a surface, scene or draw'}
if(!$prepare.Contains('SetFragmentTexture(g_probeContext,2,&g_spriteLayerTexture)') -or !$prepare.Contains('SetFragmentTexture(g_probeContext,3,&g_spriteLookupTexture)')){throw 'Completed-image priority sampler bindings lost'}
Write-Output 'PASS UI priority shader variants, bindings and completed-image submission order; device acceptance pending'
