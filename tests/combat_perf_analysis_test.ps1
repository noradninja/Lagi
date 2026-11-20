$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$runs=@(& "$repo/tools/analyze_combat_perf.ps1" -Path "$PSScriptRoot/combat_perf_synthetic.log")
if($runs.Count -ne 2){throw 'Encounter boundary count wrong'}
$first=$runs[0];$last=$runs[1]
if(!$first.ReturnedAudio -or $last.ReturnedAudio){throw 'Incomplete encounter reported as returned'}
if($first.SceneSamples -ne 2 -or $first.MedianUs.render -ne 29000 -or $first.MedianUs.cpuprep -ne 10000){throw 'Outside/GUI samples contaminated combat medians'}
if($first.FullWindows -ne 1 -or $first.Intervals -ne 120 -or $first.OverTwoVblanks -ne 80 -or $first.WindowFpsMin -ne 22){throw 'First cross-boundary window was not excluded'}
if($null -ne $first.MedianUs.positionSync -or $last.MedianUs.positionSync -ne 1900){throw 'Missing instrumentation was reported as zero or data lost'}
if($last.FullWindows -ne 0 -or $last.SceneSamples -ne 1){throw 'Trailing incomplete encounter samples lost'}
if($null -ne $first.WorkloadMedian.spriteDraws){throw 'Missing sprite instrumentation incorrectly reported as zero'}
if($last.WorkloadMedian.spriteDraws -ne 75 -or $last.WorkloadMedian.spriteWorldTagged -ne 30 -or
 $last.WorkloadMedian.spriteBboxPixels -ne 800000 -or $last.WorkloadMedian.spriteSlotSkips -ne 2){throw 'Sprite workload attribution lost or reported as timings'}
Write-Output 'PASS combat interval attribution, first-window exclusion, missing fields and incomplete encounter reporting.'
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('lagi-combat-analysis-'+[guid]::NewGuid()+'.log')
try {
 [IO.File]::WriteAllLines($fixture,@(
  '[LiveAppendPerf] frame=0 setup=999 storage=999 transform=999 light=999 binding=999 total=999 sampled=1',
  '[BattleStartup] enter BTL_A3.PRG overlay=0 sub=7',
  '[LiveAppendPerf] frame=120 setup=10 storage=20 transform=30 light=40 binding=50 total=160 sampled=1',
  '[LiveAppendPerf] frame=240 setup=30 storage=40 transform=50 light=60 binding=70 total=260 sampled=1',
  '[ScenePerf] render=30000 cpuprep=14000 obj=4500 uiOnly=0 polys=1700',
  '[ScenePerf] render=99999 uiOnly=1',
  '[FlightPresentOutlier] baseline=1 render=66000 interval=1800000 fadeR=255 fadeG=255 fadeB=255',
  '[FlightPresentOutlier] baseline=0 render=88000 interval=0 fadeR=255 fadeG=255 fadeB=255',
  '[FlightPresentOutlier] baseline=1 render=31000 interval=50000 fadeR=120 fadeG=120 fadeB=120',
  '[FlightPresentOutlier] baseline=1 render=32000 interval=50000 fadeR=0 fadeG=0 fadeB=0',
  '[FlightPresentOutlier] baseline=1 render=65000 interval=70000 fadeR=-256 fadeG=-256 fadeB=-256',
  '[FlightPresentOutlier] baseline=1 render=63000 interval=80000 fadeR=-248 fadeG=-248 fadeB=-248',
  '[FlightPresentOutlier] baseline=1 render=34000 interval=51000',
  '[FlightPresentWindow] intervals=120 over2Vblanks=120 fpsMilli=10000',
  '[FlightPresentWindow] intervals=120 over2Vblanks=30 fpsMilli=25000',
  '[BattleAudio] restore',
  '[LiveAppendPerf] frame=360 setup=999 total=999 sampled=1',
  '[BattleStartup] enter BTL_A3.PRG overlay=0 sub=5',
  '[ScenePerf] render=28000 uiOnly=0'
 ))
 $runs=(& "$repo/tools/analyze_combat_perf.ps1" -Path $fixture -AsJson | ConvertFrom-Json)
 if($runs.Count -ne 2){throw 'Encounter partition failed'}
 $first=$runs[0];$second=$runs[1]
 if($first.AppendSamples -ne 2 -or $first.SampledAppendMedianUs.setup -ne 20 -or
    $first.SampledAppendMedianUs.transform -ne 40 -or $first.SampledAppendMedianUs.total -ne 210){throw 'Sample medians or encounter boundaries failed'}
 if($first.SceneSamples -ne 1 -or $first.MedianUs.render -ne 30000){throw 'UI exclusion failed'}
 if($first.FullWindows -ne 1 -or $first.Intervals -ne 120 -or $first.OverTwoVblanks -ne 30){throw 'Partial first-window exclusion failed'}
 $phases=@{};foreach($p in $first.OutliersByFadeEndpoint){$phases[$p.Phase]=$p}
 if($phases.White.OutlierRecords -ne 2 -or $phases.White.TimedIntervals -ne 1 -or $phases.White.MaxIntervalUs -ne 1800000){throw 'White endpoint or baseline-zero attribution failed'}
 if($phases.Black.OutlierRecords -ne 1 -or $phases.VisibleOffset.OutlierRecords -ne 2 -or $phases.Neutral.OutlierRecords -ne 1 -or $phases.Unknown.OutlierRecords -ne 1){throw 'Signed fade/neutral/legacy endpoint classification failed'}
 if($null -ne $second.OutliersByFadeEndpoint[0].MaxIntervalUs){throw 'Missing outliers reported as zero'}
 if(!$first.ReturnedAudio -or $second.ReturnedAudio -or $second.AppendSamples -ne 0 -or
    $null -ne $second.SampledAppendMedianUs.transform){throw 'Incomplete encounter or absent samples failed'}
 'PASS combat analysis: sampled stage medians, encounter boundaries, UI exclusion, first partial window and absent legacy samples; synthetic fixture, not hardware evidence'
} finally {
 if(Test-Path -LiteralPath $fixture){Remove-Item -LiteralPath $fixture}
}
