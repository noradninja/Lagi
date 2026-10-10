param(
    [Parameter(Mandatory = $true)][string]$LogPath
)
$ErrorActionPreference = 'Stop'

function Read-Counters([string]$Line) {
    $result = @{}
    foreach ($match in [regex]::Matches($Line, '(\w+)=(\d+)')) {
        $result[$match.Groups[1].Value] = [long]$match.Groups[2].Value
    }
    return $result
}

function Get-Distribution($Records, [string]$Key) {
    $values = @($Records | Where-Object { $_.ContainsKey($Key) } |
        ForEach-Object { $_[$Key] } | Sort-Object)
    if ($values.Count -eq 0) { return $null }
    $middle = [int][Math]::Floor($values.Count / 2)
    $median = $values[$middle]
    if ($values.Count % 2 -eq 0) {
        $median = ($values[$middle - 1] + $values[$middle]) / 2.0
    }
    return [ordered]@{
        count = $values.Count
        medianUs = $median
        p90Us = $values[[int][Math]::Ceiling($values.Count * 0.9) - 1]
        maxUs = $values[-1]
    }
}

function Sum-Counter($Records, [string]$Key) {
    $sum = 0L
    foreach ($record in $Records) {
        if ($record.ContainsKey($Key)) { $sum += $record[$Key] }
    }
    return $sum
}

function Test-MedianBudgetBound([long]$Frames, [long]$OverBudget) {
    return $Frames -gt 0 -and 2 * $OverBudget -lt $Frames
}

$file = Get-Item -LiteralPath $LogPath
$scene = @()
$timing = @()
$present = @()
$outliers = @()
foreach ($line in Get-Content -LiteralPath $file.FullName) {
    if ($line.Contains('[ScenePerf]')) { $scene += ,(Read-Counters $line) }
    elseif ($line.Contains('[FlightTimingWindow]')) { $timing += ,(Read-Counters $line) }
    elseif ($line.Contains('[FlightPresentWindow]')) { $present += ,(Read-Counters $line) }
    elseif ($line.Contains('[FlightPresentOutlier]')) { $outliers += ,(Read-Counters $line) }
}
$groups = @()
foreach ($enabled in @(0, 1)) {
    $records = @($scene | Where-Object { $_['rbgEnabled'] -eq $enabled })
    $groups += [ordered]@{
        rbgEnabled = $enabled
        render = Get-Distribution $records 'render'
        build = Get-Distribution $records 'build'
        gxmFinish = Get-Distribution $records 'gxmfinish'
        cpuPrep = Get-Distribution $records 'cpuprep'
    }
}
$completedFrames = Sum-Counter $timing 'frames'
$completedOver25 = Sum-Counter $timing 'over25'
# Strictly fewer than half above the budget guarantees both middle values
# (for an even count) are within it. Equality does not establish that bound.
$medianBoundProven = Test-MedianBudgetBound $completedFrames $completedOver25
[ordered]@{
    path = $file.FullName
    bytes = $file.Length
    lastWriteTimeUtc = $file.LastWriteTimeUtc.ToString('o')
    scope = 'ScenePerf is sampled across native scenes, not field-only. Windows omit incomplete tails. No exact all-frame median is available.'
    sampledNativeScenes = $groups
    completeFieldRenderWindows = [ordered]@{
        windows = $timing.Count
        frames = $completedFrames
        over25ms = $completedOver25
        medianAtMost25msProven = $medianBoundProven
        medianScope = 'Only completed field windows; not an exact median or whole-capture proof.'
        over33333us = Sum-Counter $timing 'over33333'
        maxRenderUs = ($timing | ForEach-Object { $_['max'] } | Measure-Object -Maximum).Maximum
    }
    completeFieldPresentationWindows = [ordered]@{
        windows = $present.Count
        intervals = Sum-Counter $present 'intervals'
        overTwoVblanks = Sum-Counter $present 'over2Vblanks'
        maxIntervalUs = ($present | ForEach-Object { $_['max'] } | Measure-Object -Maximum).Maximum
    }
    outliers = @($outliers | ForEach-Object {
        [ordered]@{
            baseline = $_['baseline']; renderUs = $_['render']; buildUs = $_['build']
            intervalUs = $_['interval']; vblanks = $_['vblanks']; rebuild = $_['rebuild']
            gxmEndUs = $_['gxmend']; gxmFinishUs = $_['gxmfinish']
            cpuPrepUs = $_['cpuprep']; submitUs = $_['submit']; composeUs = $_['compose']
        }
    })
    interpretation = 'Missing stage fields remain null. CPU prep includes other stages; do not sum overlapping costs. GXM finish is a completion wait, not direct GPU execution time. Package SHA must be supplied separately.'
} | ConvertTo-Json -Depth 8
