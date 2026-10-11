$ErrorActionPreference = 'Stop'
# Geometry arithmetic and source-order guards only, not a Vita playback test.
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/platform/vita/neptune_renderer.cpp')
$runtime = Get-Content -Raw (Join-Path $PSScriptRoot '../src/integration/lagi_runtime.cpp')
$output = @(640,368)
foreach ($mode in @(0,1)) {
    foreach ($input in @(@(320,176), @(304,160), @(320,240), @(480,272))) {
        $aspect = $output[0] / [double]$output[1]
        $sourceAspect = $input[0] / [double]$input[1]
        $xExtent = 2.0 * $input[0] / $output[0]
        $yExtent = 2.0 * $input[1] / $output[1]
        $width = $xExtent * $output[0]
        $height = $yExtent * $output[1]
        if ($width -ne 2*$input[0] -or $height -ne 2*$input[1] -or [Math]::Abs($width/$height-$sourceAspect) -gt 1e-10) { throw 'Integer-scale aspect mismatch' }
    }
}
$start = $source.IndexOf('bool movie_present_cinepak_payload(')
$end = $source.IndexOf('bool movie_republish_frame()', $start)
$payload = $source.Substring($start, $end-$start)
if ($payload -notmatch '2.0f \* float\(sourceWidth\) / float\(kCinepakOutputWidth\)' -or
    $payload -notmatch '2.0f \* float\(sourceHeight\) / float\(kCinepakOutputHeight\)') { throw 'Cinepak 2x geometry missing' }
if ($payload.IndexOf('publishMovieOverlaySnapshot();') -gt $payload.IndexOf('renderSlot.publish()') -or
    !$payload.Contains('publishMovieOverlaySnapshot();')) { throw 'Snapshot must precede render-slot publication' }
$start = $source.IndexOf('static bool renderMovieFrame()')
$end = $source.IndexOf('static void renderBasicWingViewer', $start)
if ($end -lt 0) { $end = $source.Length }
$movie = $source.Substring($start, $end-$start)
$bars = $movie.IndexOf('drawAzelVdp2CinematicBarsGpu();')
$text = $movie.IndexOf('drawAzelVdp2TextLayerGpu(movieOutputWidth, movieOutputHeight);')
$fade = $movie.IndexOf('drawAzelFrontendFadeOffset();')
if ($bars -lt 0 -or $text -le $bars -or $fade -le $text) { throw 'Expected video -> matte -> text -> fade ordering' }
if ($runtime -notmatch '(?s)interruptVDP2Update\(\);\s*if \(gGameStatus.m0_gameMode == 0\).*?presentation_set_vdp2_text\(.*?lastUpdateFunction\(\);') { throw 'Movie-mode overlays must be staged after DMA and before movie service' }
if ($source -notmatch 'kCinepakOutputWidth = 640' -or $source -notmatch 'kCinepakOutputHeight = 368' -or
    $movie -notmatch 'frameBuffer.width = movieOutputWidth' -or
    $movie -notmatch 'cinepakPresentation \? g_cinepakPresentationRenderTarget' -or
    $movie -notmatch 'setStochastic\(g_movieWidth, g_movieHeight\)' -or
    $movie -notmatch 'sourceTexture, SCE_GXM_TEXTURE_FILTER_POINT') { throw '640x368 direct scanout/dither guards failed' }
Write-Output 'PASS: 2x geometry, 640x368 scanout selection, dither retention and movie overlay publication/order (CPU/source checks only)'
