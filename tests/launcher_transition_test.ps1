$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$source = Get-Content "$root/src/platform/vita/neptune_renderer.cpp" -Raw
$shader = Get-Content "$root/shaders/rgba_present_f.cg" -Raw
if ($source -notmatch 'g_preserveLoadingScanout \? 0 : sceDisplaySetFrameBuf') {
    throw 'Backend initialization must preserve the loading scanout.'
}
$prepare = $source.Substring($source.IndexOf('static bool launcherPrepareGpu'))
if ($prepare.IndexOf('std::memcpy(g_launcherLoading.pixels') -gt $prepare.IndexOf('launcherTextured(available, selection)')) {
    throw 'Loading capture must precede selector composition.'
}
$fade = $source.Substring($source.IndexOf('void launcher_fade'), $source.IndexOf('void launcher_present') - $source.IndexOf('void launcher_fade'))
if ($fade -notmatch 'Hold the completed black GXM frame until native presentation') {
    throw 'Final black frame must retain GXM ownership.'
}
if ($shader -notmatch 'return tex2D\(raw,vTexcoord\)\*presentInfo.x') {
    throw 'Crossfade must scale premultiplied RGB and alpha together.'
}
for ($frame = 0; $frame -le 30; ++$frame) {
    $alpha = (30 - $frame) / 30.0
    if ($alpha -lt 0 -or $alpha -gt 1) { throw 'Invalid transition opacity.' }
    if ($frame -eq 0 -and $alpha -ne 1) { throw 'Loading endpoint missing.' }
    if ($frame -eq 30 -and $alpha -ne 0) { throw 'Selector endpoint missing.' }
}
Write-Output 'PASS: loading capture order, GPU crossfade endpoints, black handoff contract (not hardware validation)'
