param([Parameter(Mandatory=$true)][string]$Path,[switch]$AsJson)
$ErrorActionPreference='Stop'
function Read-Fields([string]$Line){
 $fields=@{}
 foreach($match in [regex]::Matches($Line,'([A-Za-z0-9_]+)=(-?\d+)')){
  $fields[$match.Groups[1].Value]=[long]$match.Groups[2].Value
 }
 return $fields
}
function Get-Median($Values){
 $sorted=@($Values | Sort-Object)
 if(!$sorted.Count){return $null}
 $middle=[int][Math]::Floor($sorted.Count/2)
 if($sorted.Count%2){return $sorted[$middle]}
 return ($sorted[$middle-1]+$sorted[$middle])/2.0
}
$runs=[Collections.Generic.List[object]]::new()
$active=$null;$lineNumber=0
foreach($line in [IO.File]::ReadLines((Resolve-Path -LiteralPath $Path).Path)){
 ++$lineNumber
 if($line.Contains('[BattleStartup] enter')){
  $f=Read-Fields $line
  $active=@{Start=$lineNumber;End=$null;Sub=$f.sub;Overlay=$f.overlay;Samples=[Collections.Generic.List[object]]::new();AppendSamples=[Collections.Generic.List[object]]::new();Windows=[Collections.Generic.List[object]]::new();Outliers=[Collections.Generic.List[object]]::new();FirstWindow=$true}
  $runs.Add($active)
 }
 if($null -eq $active){continue}
 if($line.Contains('[LiveAppendPerf]')){$active.AppendSamples.Add((Read-Fields $line))}
 if($line.Contains('[FlightPresentOutlier]')){
  $f=Read-Fields $line
  $phase='Unknown'
  if($f.ContainsKey('fadeR') -and $f.ContainsKey('fadeG') -and $f.ContainsKey('fadeB')){
   if($f.fadeR -ge 255 -and $f.fadeG -ge 255 -and $f.fadeB -ge 255){$phase='White'}
   elseif($f.fadeR -le -255 -and $f.fadeG -le -255 -and $f.fadeB -le -255){$phase='Black'}
   elseif($f.fadeR -eq 0 -and $f.fadeG -eq 0 -and $f.fadeB -eq 0){$phase='Neutral'}
   else{$phase='VisibleOffset'}
  }
  $active.Outliers.Add([pscustomobject]@{Phase=$phase;Fields=$f})
 }
 if($line.Contains('[ScenePerf]')){
  $f=Read-Fields $line
  if(!$f.uiOnly){$active.Samples.Add($f)}
 }
 if($line.Contains('[FlightPresentWindow]')){
  # First emitted window can include pre-encounter frames; never count it as
  # a complete combat window. Renderer mode numbers are NOT battle identity.
  if($active.FirstWindow){$active.FirstWindow=$false}else{$active.Windows.Add((Read-Fields $line))}
 }
 if($line.Contains('[BattleAudio] restore')){$active.End=$lineNumber;$active=$null}
}
$result=@(foreach($run in $runs){
 $median=[ordered]@{}
 foreach($field in @('render','cpuprep','gxmfinish','build','obj','upload','light','gourPrep','submit','compose','clear','rbgResolve','tasks','positionSync')){
  $values=@($run.Samples | Where-Object {$_.ContainsKey($field)} | ForEach-Object {$_[$field]})
  $median[$field]=Get-Median $values
 }
 $workload=[ordered]@{}
 $append=[ordered]@{}
 foreach($field in @('setup','storage','transform','light','binding','total')){
  $values=@($run.AppendSamples | Where-Object {$_.ContainsKey($field)} | ForEach-Object {$_[$field]})
  $append[$field]=Get-Median $values
 }
 foreach($field in @('polys','verts','positionSyncVerts','submissions','spriteDraws','spriteWorldTagged','spriteShaded','spriteBboxPixels','spriteCacheHits','spriteCacheMisses','spritePaletteRefresh','spriteSlotSkips')){
  $values=@($run.Samples | Where-Object {$_.ContainsKey($field)} | ForEach-Object {$_[$field]})
  $workload[$field]=Get-Median $values
 }
 $intervals=0L;$missed=0L
 foreach($window in $run.Windows){$intervals+=$window.intervals;$missed+=$window.over2Vblanks}
 $fps=@($run.Windows | ForEach-Object {$_.fpsMilli/1000.0})
 $phases=@(foreach($phase in @('White','Black','Neutral','VisibleOffset','Unknown')){
  $entries=@($run.Outliers | Where-Object {$_.Phase -eq $phase})
  $timed=@($entries | Where-Object {$_.Fields.ContainsKey('baseline') -and $_.Fields.baseline -eq 1 -and $_.Fields.ContainsKey('interval')} | ForEach-Object {$_.Fields.interval})
  [pscustomobject]@{
   Phase=$phase;OutlierRecords=$entries.Count;TimedIntervals=$timed.Count
   MaxIntervalUs=($timed | Measure-Object -Maximum).Maximum
   MedianRenderUs=(Get-Median @($entries | Where-Object {$_.Fields.ContainsKey('render')} | ForEach-Object {$_.Fields.render}))
  }
 })
 [pscustomobject]@{
  Overlay=$run.Overlay;Sub=$run.Sub;StartLine=$run.Start;EndLine=$run.End;ReturnedAudio=($null -ne $run.End)
  SceneSamples=$run.Samples.Count;FullWindows=$run.Windows.Count;Intervals=$intervals;OverTwoVblanks=$missed
  WindowFpsMin=($fps | Measure-Object -Minimum).Minimum;WindowFpsMax=($fps | Measure-Object -Maximum).Maximum
  MedianUs=[pscustomobject]$median
  WorkloadMedian=[pscustomobject]$workload
  AppendSamples=$run.AppendSamples.Count;SampledAppendMedianUs=[pscustomobject]$append
  OutliersByFadeEndpoint=$phases
  Notes='Startup-to-audio-restore span can include intro/results/loading. Outlier phases classify the published fade at the interval endpoint only, not the entire interval or battle submode. White holds remain in unmodified window statistics. Heartbeat medians are sampled, not whole-frame averages. cpuprep/build/sync overlap; GPU finish is wait time. tasks=0 in older logs is uninstrumented. ReturnedAudio does not prove encounter correctness.'
 }
})
if($AsJson){ConvertTo-Json -InputObject $result -Depth 5}else{$result}
