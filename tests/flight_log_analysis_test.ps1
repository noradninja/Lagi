$ErrorActionPreference = 'Stop'
$report = (. "$PSScriptRoot/../tools/analyze-flight-log.ps1" -LogPath "$PSScriptRoot/flight_log_fixture.txt") | ConvertFrom-Json
foreach ($case in @(
    @(0, 0, $false), @(120, 59, $true), @(120, 60, $false),
    @(120, 61, $false), @(3, 1, $true), @(3, 2, $false)
)) {
    if ((Test-MedianBudgetBound $case[0] $case[1]) -ne $case[2]) {
        throw 'Median-bound empty/even/odd threshold mismatch'
    }
}
$rbg = $report.sampledNativeScenes | Where-Object { $_.rbgEnabled -eq 1 }
if ($rbg.render.count -ne 2 -or $rbg.render.medianUs -ne 25000 -or
    $rbg.render.p90Us -ne 30000 -or $rbg.build.medianUs -ne 2000) {
    throw 'Sample distribution mismatch'
}
if ($report.completeFieldRenderWindows.frames -ne 120 -or
    -not $report.completeFieldRenderWindows.medianAtMost25msProven -or
    $report.completeFieldRenderWindows.over33333us -ne 1 -or
    $report.completeFieldPresentationWindows.overTwoVblanks -ne 0) {
    throw 'Window totals mismatch'
}
if ($report.outliers.Count -ne 2 -or $report.outliers[0].vblanks -ne 29 -or
    $null -ne $report.outliers[0].gxmFinishUs -or
    $report.outliers[1].gxmFinishUs -ne 14000) {
    throw 'Outlier retention or missing-field mismatch'
}
Write-Output 'PASS: sampled distributions, window totals, incomplete-tail outliers and missing stages'
if ($report.nativeTextureUploads.events -ne 2 -or
    $report.nativeTextureUploads.textures -ne 5 -or
    $report.nativeTextureUploads.copiedBytes -ne 12288 -or
    $report.nativeTextureUploads.allocation.medianUs -ne 150 -or
    $report.nativeTextureUploads.copy.p90Us -ne 80 -or
    $report.nativeTextureUploads.total.medianUs -ne 310) {
    throw 'Upload-stage distributions or totals mismatch'
}
Write-Output 'PASS: upload stage distributions and byte/count totals'
