$ErrorActionPreference = 'Stop'
# Geometry arithmetic and source-order guards only, not a Vita playback test.
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/platform/vita/neptune_renderer.cpp')
$runtime = Get-Content -Raw (Join-Path $PSScriptRoot '../src/integration/lagi_runtime.cpp')
foreach ($output in @(@(480,272), @(960,544))) {
    foreach ($input in @(@(320,144), @(320,240), @(480,272))) {
        $aspect = $output[0] / [double]$output[1]
        $sourceAspect = $input[0] / [double]$input[1]
        $xExtent = 1.0
        $yExtent = $aspect / $sourceAspect
        $width = $xExtent * $output[0]
        $height = $yExtent * $output[1]
        if ($width -ne $output[0] -or [Math]::Abs($width/$height-$sourceAspect) -gt 1e-10) { throw 'Width-fit aspect mismatch' }
        if (($sourceAspect -lt $aspect) -ne ($yExtent -gt 1.0)) { throw 'Vertical crop mismatch' }
    }
}
$start = $source.IndexOf('bool movie_present_cinepak_payload(')
$end = $source.IndexOf('bool movie_republish_frame()', $start)
$payload = $source.Substring($start, $end-$start)
if ($payload -notmatch 'const float xExtent = 1.0f;' -or
    $payload -notmatch 'xExtent \* displayAspect / sourceAspect') { throw 'Cinepak width-fit geometry missing' }
if ($payload.IndexOf('publishMovieOverlaySnapshot();') -gt $payload.IndexOf('renderSlot.publish()') -or
    !$payload.Contains('publishMovieOverlaySnapshot();')) { throw 'Snapshot must precede render-slot publication' }
$start = $source.IndexOf('static bool renderMovieFrame()')
$end = $source.IndexOf('static void renderBasicWingViewer', $start)
if ($end -lt 0) { $end = $source.Length }
$movie = $source.Substring($start, $end-$start)
$bars = $movie.IndexOf('drawAzelVdp2CinematicBarsGpu();')
$text = $movie.IndexOf('drawAzelVdp2TextLayerGpu();')
$fade = $movie.IndexOf('drawAzelFrontendFadeOffset();')
if ($bars -lt 0 -or $text -le $bars -or $fade -le $text) { throw 'Expected video -> matte -> text -> fade ordering' }
if ($runtime -notmatch '(?s)interruptVDP2Update\(\);\s*if \(gGameStatus.m0_gameMode == 0\).*?presentation_set_vdp2_text\(.*?lastUpdateFunction\(\);') { throw 'Movie-mode overlays must be staged after DMA and before movie service' }
Write-Output 'PASS: width fit/aspect/crop geometry and movie overlay publication/order (CPU/source checks only)'
