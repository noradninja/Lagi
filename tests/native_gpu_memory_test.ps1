$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$renderer.IndexOf('static bool prepareSceneVdp2Background()' + "`n" + '{')
if($start -lt 0){$start=$renderer.IndexOf('static bool prepareSceneVdp2Background()' + "`r`n" + '{')}
$body=$renderer.Substring($start)
$local=$body.IndexOf('g_sceneVdp2GpuRaw = probeCdramAlloc(kRawVdp2Bytes,')
$fallback=$body.IndexOf('g_sceneVdp2GpuRaw=probeGpuAlloc(kRawVdp2Bytes,')
$texture=$body.IndexOf('sceGxmTextureInitLinear(&g_sceneVdp2Texture, g_sceneVdp2GpuRaw,')
if($local -lt 0 -or $fallback -lt $local -or $texture -lt $fallback){throw 'RBG local allocation/fallback/descriptor order lost'}
if(!$body.Contains('freeMovieMappedBlock(g_sceneVdp2RawUid, g_sceneVdp2GpuRaw)') -or !$body.Contains('[RBG0Memory] cdram=%u')){throw 'RBG cleanup or placement telemetry lost'}
Write-Output 'PASS native RBG GPU-local allocation, fallback and cleanup contracts'
$world=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
$local=$world.IndexOf('g_spriteLayerPixels=probeCdramAlloc(kGxmPitch*kHeight*4,')
$fallback=$world.IndexOf('g_spriteLayerPixels=probeGpuAlloc(kGxmPitch*kHeight*4,')
$surface=$world.IndexOf('sceGxmColorSurfaceInit(&g_spriteLayerSurface,')
if($local -lt 0 -or $fallback -lt $local -or $surface -lt $fallback){throw 'World surface local allocation/fallback/descriptor order lost'}
if(!$world.Contains('freeMovieMappedBlock(g_spriteLayerUid,g_spriteLayerPixels)') -or !$world.Contains('[VDP2SpriteMemory] cdram=%u')){throw 'World surface cleanup or placement telemetry lost'}
Write-Output 'PASS native world surface GPU-local allocation, fallback and cleanup contracts'
$frontend=$renderer.Substring($renderer.IndexOf('bool frontend_present_vdp2('))
$local=$frontend.IndexOf('g_movieTextureData = probeCdramAlloc(')
$fallback=$frontend.IndexOf('g_movieTextureData=probeGpuAlloc(')
$texture=$frontend.IndexOf('if (sceGxmTextureInitLinear(')
if($local -lt 0 -or $fallback -lt $local -or $texture -lt $fallback -or !$frontend.Contains('[FrontendVdp2Memory] cdram=%u')){throw 'Frontend VDP2 local allocation/fallback/descriptor order lost'}
$other=$renderer.Substring($renderer.IndexOf('bool movie_'),$renderer.IndexOf('bool frontend_present_vdp2(')-$renderer.IndexOf('bool movie_'))
if($other.Contains('g_movieTextureData = probeCdramAlloc(')){throw 'Non-VDP2 movie allocation changed'}
Write-Output 'PASS frontend VDP2-only local allocation and fallback contracts'
