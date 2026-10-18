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
Write-Output 'PASS combat interval attribution, first-window exclusion, missing fields and incomplete encounter reporting.'
