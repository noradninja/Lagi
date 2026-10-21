param([Parameter(Mandatory=$true)][string]$LogPath)
$ErrorActionPreference='Stop'
function Read-Vdp2Counters([string]$Line) {
    $f=@{}
    foreach($m in [regex]::Matches($Line,'(\w+)=(\d+)')){$f[$m.Groups[1].Value]=[long]$m.Groups[2].Value}
    return $f
}
function Get-Vdp2Distribution($Records,[string]$Field) {
    $v=@($Records | Where-Object {$_.ContainsKey($Field)} | ForEach-Object {$_[$Field]} | Sort-Object)
    if(!$v.Count){return $null}
    $mid=[int][Math]::Floor($v.Count/2)
    $median=if($v.Count%2){$v[$mid]}else{($v[$mid-1]+$v[$mid])/2.0}
    return [ordered]@{count=$v.Count;medianUs=$median;p90Us=$v[[int][Math]::Ceiling($v.Count*.9)-1];maxUs=$v[-1]}
}
function Get-Vdp2Label($Fields,[string]$Name) {
    if($Fields.ContainsKey($Name)){return [string]$Fields[$Name]}
    return 'unknown'
}
$file=Get-Item -LiteralPath $LogPath
$initialHash=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
$frontend=[Collections.Generic.List[object]]::new()
$scene=[Collections.Generic.List[object]]::new()
$pages=[Collections.Generic.List[object]]::new()
$passes=[Collections.Generic.List[object]]::new()
$markers=[Collections.Generic.List[string]]::new()
foreach($line in [IO.File]::ReadLines($file.FullName)) {
    if($line.Contains('[VDP2Perf]')) {
        $f=Read-Vdp2Counters $line
        $fb=[regex]::Match($line,'fb=(\d+)x(\d+)')
        $f['framebuffer']=if($fb.Success){$fb.Groups[1].Value+'x'+$fb.Groups[2].Value}else{'unknown'}
        $f['_group']='layout='+(Get-Vdp2Label $f 'layout')+' nbg='+(Get-Vdp2Label $f 'nbg')+' fb='+$f.framebuffer
        $frontend.Add($f)
    } elseif($line.Contains('[ScenePerf]')) {
        $f=Read-Vdp2Counters $line
        $f['_group']='rbg='+(Get-Vdp2Label $f 'rbgEnabled')+' uiOnly='+(Get-Vdp2Label $f 'uiOnly')+' serialized='+(Get-Vdp2Label $f 'gpuSerialized')
        $scene.Add($f)
    } elseif($line.Contains('[RBG0PageCache]')) {
        $f=Read-Vdp2Counters $line
        $f['_group']='frontend='+(Get-Vdp2Label $f 'frontend')+' active='+(Get-Vdp2Label $f 'active')+' reason='+(Get-Vdp2Label $f 'reason')
        $pages.Add($f)
    } elseif($line.Contains('[GpuPassTiming]')) {$passes.Add((Read-Vdp2Counters $line))}
    if($line -match '\[(NeptuneBuild|FrontendRbgResolve|RBG0WindowCoverage|RBG0CoefficientRows|RBG0TileFormat)\]') {
        if(!$markers.Contains($line)){$markers.Add($line)}
    }
}
$frontendGroups=@(foreach($group in @($frontend | Group-Object -Property '_group' | Sort-Object Name)) {
    $records=@($group.Group)
    $frames=@($records | Where-Object {$_.ContainsKey('frame')})
    [ordered]@{
        group=$group.Name
        render=Get-Vdp2Distribution $records 'frame'
        finalCompletionWait=Get-Vdp2Distribution $records 'gpuWait'
        over25msSamples=@($frames | Where-Object {$_['frame'] -gt 25000}).Count
        over33msSamples=@($frames | Where-Object {$_['frame'] -gt 33000}).Count
        scope='Sampled frontend render work before pacing/presentation. Final wait is included in frame; never add them. Missing nbg remains unknown.'
    }
})
$sceneGroups=@(foreach($group in @($scene | Group-Object -Property '_group' | Sort-Object Name)) {
    $records=@($group.Group)
    $stages=[ordered]@{}
    foreach($field in @('render','tasks','build','cpuprep','gxmend','gxmfinish','rbgPrep','rbgResolve')) {
        $stages[$field]=Get-Vdp2Distribution $records $field
    }
    [ordered]@{
        group=$group.Name;timingUs=$stages
        over33msRenderSamples=@($records | Where-Object {$_.ContainsKey('render') -and $_['render'] -gt 33000}).Count
        scope='Sampled native scenes, not encounter/field identity. Task/render and nested CPU timings can overlap; do not sum. Missing fields remain null.'
    }
})
$cacheGroups=@(foreach($group in @($pages | Group-Object -Property '_group' | Sort-Object Name)) {
    $records=@($group.Group)
    $hits=@($records | Where-Object {$_.ContainsKey('rebuilt') -and $_['rebuilt'] -eq 0 -and $_.ContainsKey('hits') -and $_['hits'] -gt 0})
    $withBytes=@($records | Where-Object {$_.ContainsKey('uploadBytes')})
    [ordered]@{
        group=$group.Name;loggedEvents=$records.Count;hitEvents=$hits.Count
        hitPrep=Get-Vdp2Distribution $hits 'prepUs'
        allEventPrep=Get-Vdp2Distribution $records 'prepUs'
        loggedUploadBytes=if($withBytes.Count){($withBytes | ForEach-Object {$_['uploadBytes']} | Measure-Object -Sum).Sum}else{$null}
        scope='Rate-limited events, not every cache operation. Logged bytes are not total capture uploads. Missing source domain remains unknown.'
    }
})
$native=(& "$PSScriptRoot/analyze-flight-log.ps1" -LogPath $file.FullName) | ConvertFrom-Json
$finalFile=Get-Item -LiteralPath $file.FullName
$finalHash=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
if($file.Length -ne $finalFile.Length -or $file.LastWriteTimeUtc -ne $finalFile.LastWriteTimeUtc -or $initialHash -ne $finalHash){
    throw 'Log changed during analysis. Retry after the capture copy is complete.'
}
[ordered]@{
    log=$file.FullName;bytes=$file.Length;sha256=$finalHash
    buildMarkers=@($markers)
    frontend=$frontendGroups;nativeSceneSamples=$sceneGroups;pageCacheEvents=$cacheGroups
    serializedGpuPassSamples=@($passes)
    fieldRenderWindows=$native.completeFieldRenderWindows
    fieldPresentationWindows=$native.completeFieldPresentationWindows
    presentationOutliers=$native.outliers
    acceptance='Not established automatically: samples omit frames, completed windows omit tails, device revision is not recorded, and visual/route acceptance must be supplied separately.'
    interpretation='Budgets are exactly 25000/33000 us for sampled work. 30 Hz presentation pacing is separate. GPU completion waits include queued work/scheduling, not isolated GPU timestamps. Serialized probes intentionally change timing.'
} | ConvertTo-Json -Depth 10
