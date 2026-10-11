$ErrorActionPreference = 'Stop'
# CPU layout and source guard only; not a hardware alignment/image test.
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/platform/vita/neptune_renderer.cpp')
$match = [regex]::Match($source, '(?s)if \(fieldRadarMap\) \{\s*// Keep the established layout size:.*?\n\s*\} else')
if (!$match.Success) { throw 'Radar geometry branch missing' }
$geometry = $match.Value
$widthMatch = [regex]::Match($geometry, 'radarReferenceWidth = ([0-9.]+)f')
$heightMatch = [regex]::Match($geometry, 'radarReferenceHeight = ([0-9.]+)f')
if (!$widthMatch.Success -or !$heightMatch.Success) { throw 'Radar reference dimensions missing' }
$referenceWidth = [double]$widthMatch.Groups[1].Value
$referenceHeight = [double]$heightMatch.Groups[1].Value
if ($geometry -match 'viewerRenderWidth|viewerRenderHeight' -or
    $geometry -notmatch '/\s*radarReferenceWidth' -or
    $geometry -notmatch '/\s*radarReferenceHeight') { throw 'Radar size must use reference layout dimensions' }
foreach ($case in @(@(480,272,48), @(960,544,96))) {
    $width = (2.0 * 48.0 / $referenceWidth) * $case[0] / 2.0
    $height = (2.0 * 48.0 / $referenceHeight) * $case[1] / 2.0
    if ($width -ne $case[2] -or $height -ne $case[2]) { throw "Unexpected radar size $width x $height" }
}
$pointBranch = [regex]::Match($source, '(?s)if \(fieldRadarMap\) \{\s*sceGxmTextureSetMinFilter.*?\n\s*\}').Value
if ([regex]::Matches($pointBranch, 'SCE_GXM_TEXTURE_FILTER_POINT').Count -ne 2) { throw 'Radar point-filter exception changed' }
Write-Output 'PASS: radar 48x48 at 480x272, 96x96 at 960x544; point-filter exception retained (CPU/source checks only)'
