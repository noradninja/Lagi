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
