$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('lagi-combat-analysis-'+[guid]::NewGuid()+'.log')
try {
 [IO.File]::WriteAllLines($fixture,@(
  '[LiveAppendPerf] frame=0 setup=999 storage=999 transform=999 light=999 binding=999 total=999 sampled=1',
  '[BattleStartup] enter BTL_A3.PRG overlay=0 sub=7',
  '[LiveAppendPerf] frame=120 setup=10 storage=20 transform=30 light=40 binding=50 total=160 sampled=1',
  '[LiveAppendPerf] frame=240 setup=30 storage=40 transform=50 light=60 binding=70 total=260 sampled=1',
  '[ScenePerf] render=30000 cpuprep=14000 obj=4500 uiOnly=0 polys=1700',
  '[ScenePerf] render=99999 uiOnly=1',
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
 if(!$first.ReturnedAudio -or $second.ReturnedAudio -or $second.AppendSamples -ne 0 -or
    $null -ne $second.SampledAppendMedianUs.transform){throw 'Incomplete encounter or absent samples failed'}
 'PASS combat analysis: sampled stage medians, encounter boundaries, UI exclusion, first partial window and absent legacy samples; synthetic fixture, not hardware evidence'
} finally {
 if(Test-Path -LiteralPath $fixture){Remove-Item -LiteralPath $fixture}
}
