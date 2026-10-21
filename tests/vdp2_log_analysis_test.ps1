$ErrorActionPreference='Stop'
$report=(& "$PSScriptRoot/../tools/analyze-vdp2-log.ps1" -LogPath "$PSScriptRoot/vdp2_log_fixture.txt") | ConvertFrom-Json
$unknown=@($report.frontend | Where-Object {$_.group -eq 'layout=2 nbg=unknown fb=960x544'})
if($report.frontend.Count -ne 4 -or $unknown.Count -ne 1 -or
   $unknown[0].render.medianUs -ne 33000 -or $unknown[0].finalCompletionWait.count -ne 1 -or
   $unknown[0].over33msSamples -ne 1){throw 'Frontend grouping/missing wait/budget mismatch'}
$hidden=$report.frontend | Where-Object {$_.group -eq 'layout=2 nbg=0 fb=960x544'}
$shown=$report.frontend | Where-Object {$_.group -eq 'layout=2 nbg=1 fb=960x544'}
if($hidden.render.medianUs -ne 25500 -or $hidden.over25msSamples -ne 1 -or
   $hidden.over33msSamples -ne 0 -or $shown.render.medianUs -ne 50000){throw 'Visibility or inclusive budget boundary mismatch'}
$cache=$report.pageCacheEvents | Where-Object {$_.group -eq 'frontend=1 active=1 reason=0'}
if($cache.loggedEvents -ne 3 -or $cache.hitEvents -ne 2 -or $cache.hitPrep.medianUs -ne 21 -or
   $cache.loggedUploadBytes -ne 1048576 -or $report.pageCacheEvents.Count -ne 4){throw 'Cache cold/hit/domain grouping mismatch'}
$failedCache=$report.pageCacheEvents | Where-Object {$_.group -eq 'frontend=1 active=0 reason=3'}
if($null -ne $failedCache.hitPrep -or $null -ne $failedCache.loggedUploadBytes){throw 'Missing counters became zero'}
$native=$report.nativeSceneSamples | Where-Object {$_.group -eq 'rbg=1 uiOnly=0 serialized=0'}
$legacy=$report.nativeSceneSamples | Where-Object {$_.group -eq 'rbg=0 uiOnly=unknown serialized=unknown'}
if($native.timingUs.render.medianUs -ne 33000 -or $native.timingUs.tasks.medianUs -ne 6250 -or
   $native.over33msRenderSamples -ne 1 -or $null -ne $legacy.timingUs.tasks -or
   $report.serializedGpuPassSamples.Count -ne 1){throw 'Native scope/stall/missing-task mismatch'}
if($report.fieldRenderWindows.frames -ne 120 -or $report.fieldRenderWindows.medianAtMost25msProven -or
   $report.fieldPresentationWindows.overTwoVblanks -ne 1 -or $report.presentationOutliers[0].baseline -ne 0){
    throw 'Render/presentation windows or incomplete-tail outlier lost'
}
if($report.sha256.Length -ne 64 -or $report.buildMarkers.Count -ne 2 -or
   !$report.acceptance.StartsWith('Not established automatically')){throw 'Evidence limitations/hash missing'}
$empty=(& "$PSScriptRoot/../tools/analyze-vdp2-log.ps1" -LogPath "$PSScriptRoot/vdp2_log_no_samples.txt") | ConvertFrom-Json
if($empty.frontend.Count -ne 0 -or $empty.nativeSceneSamples.Count -ne 0 -or
   $empty.fieldRenderWindows.frames -ne 0 -or $empty.fieldRenderWindows.medianAtMost25msProven){
    throw 'Empty capture fabricated timing or acceptance'
}
Write-Output 'PASS VDP2 log grouping, missing fields, exact budgets, cold/hit events, serialized isolation, windows and conservative acceptance scope'
