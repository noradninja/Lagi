param([Parameter(Mandatory=$true)][string]$Path)
$ErrorActionPreference = 'Stop'

function Read-Fields([string]$Line) {
    $fields = @{}
    foreach ($match in [regex]::Matches($Line, '([A-Za-z0-9_]+)=(-?\d+)')) {
        $fields[$match.Groups[1].Value] = [long]$match.Groups[2].Value
    }
    return $fields
}

function Median([object[]]$Values) {
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -eq 0) { return $null }
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

$samples = [Collections.Generic.List[object]]::new()
$inBattle = $false
foreach ($line in [IO.File]::ReadLines((Resolve-Path -LiteralPath $Path).Path)) {
    if ($line.Contains('[BattleStartup] enter')) { $inBattle = $true }
    if ($line.Contains('[BattleAudio] restore')) { $inBattle = $false }
    if (!$inBattle -or !$line.Contains('[ScenePerf]')) { continue }
    $fields = Read-Fields $line
    if (!$fields.ContainsKey('transientMax') -or
        !$fields.ContainsKey('render') -or
        $fields.uiOnly -ne 0 -or
        $fields.staticRebuilt -ne 0 -or
        $fields.rbgEnabled -ne 1) { continue }
    $samples.Add($fields)
}

if ($samples.Count -eq 0) {
    Write-Warning 'No eligible diagnostic ScenePerf samples. Use the transientMax build, with a battle route and no UI/static-rebuild samples.'
    return
}

$fieldsToReport = @('polys','submissions','spriteDraws','transientPolys',
    'transientTests','transientHits','build','obj','light','gourPrep',
    'gourPayload','submit','compose','cpuprep','gxmfinish','render')
foreach ($bucket in @('0','1','2+')) {
    $group = @($samples | Where-Object {
        if ($bucket -eq '2+') { $_.transientMax -ge 2 }
        elseif ($bucket -eq '1') { $_.transientMax -eq 1 }
        else { $_.transientMax -eq 0 }
    })
    if ($group.Count -eq 0) { continue }
    $result = [ordered]@{
        MaxLightsPerLitPolygon = $bucket
        Samples = $group.Count
        Over30ms = @($group | Where-Object { $_.render -gt 30000 }).Count
    }
    foreach ($field in $fieldsToReport) {
        $result["${field}Median"] = Median @($group | Where-Object {
            $_.ContainsKey($field)
        } | ForEach-Object { $_[$field] })
    }
    [pscustomobject]$result
}

Write-Warning 'ScenePerf is a sparse heartbeat. Buckets can differ in geometry, sprites, and encounter phase; medians are correlations, not isolated light costs. Timings are microseconds except counts.'
