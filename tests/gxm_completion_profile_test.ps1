$ErrorActionPreference = 'Stop'

$rendererPath = Join-Path $PSScriptRoot '..\src\platform\vita\neptune_renderer.cpp'
$renderer = Get-Content -LiteralPath $rendererPath -Raw

if ($renderer -notmatch 'g_profileGxmEndSceneUs') {
    throw 'Renderer does not retain a separate sceGxmEndScene timing'
}
if ($renderer -notmatch 'g_profileGxmFinishUs') {
    throw 'Renderer does not retain a separate sceGxmFinish timing'
}

$endScene = $renderer.IndexOf('sceGxmEndScene(g_probeContext')
$finish = $renderer.IndexOf('sceGxmFinish(g_probeContext)')
$combined = $renderer.IndexOf('g_profileGxmWaitUs = static_cast<unsigned int>')
if ($endScene -lt 0 -or $finish -le $endScene -or $combined -le $finish) {
    throw 'GXM completion timings are not recorded in submission order'
}

if ($renderer -notmatch 'gxmwait=%u gxmend=%u gxmfinish=%u') {
    throw 'ScenePerf does not publish the split GXM completion timings'
}

Write-Host 'GXM completion profile checks passed.'
