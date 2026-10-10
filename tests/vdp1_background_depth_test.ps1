$ErrorActionPreference = 'Stop'

function Is-Background([bool]$HasDepth, [single]$Depth) {
    return $HasDepth -and $Depth -ge [single]0.999
}

$cases = @(
    @{ HasDepth = $false; Depth = 1.0; Expected = $false },
    @{ HasDepth = $true;  Depth = 0.0; Expected = $false },
    @{ HasDepth = $true;  Depth = 0.5; Expected = $false },
    @{ HasDepth = $true;  Depth = 0.998; Expected = $false },
    @{ HasDepth = $true;  Depth = 0.999; Expected = $true },
    @{ HasDepth = $true;  Depth = 1.0; Expected = $true }
)
foreach ($case in $cases) {
    $actual = Is-Background $case.HasDepth $case.Depth
    if ($actual -ne $case.Expected) {
        throw "Depth classification mismatch: hasDepth=$($case.HasDepth) depth=$($case.Depth)"
    }
}

$repo = Split-Path $PSScriptRoot -Parent
$bridgeHeader = Get-Content "$repo\include\lagi\lagi_render_bridge.h" -Raw
$runtime = Get-Content "$repo\src\integration\lagi_runtime.cpp" -Raw
$renderer = Get-Content "$repo\src\platform\vita\neptune_renderer.cpp" -Raw

if ($bridgeHeader -notmatch 'float\s+depth\s*=\s*0\.0f' -or
    $bridgeHeader -notmatch 'bool\s+hasDepth\s*=\s*false') {
    throw 'VDP1 bridge no longer publishes authored depth'
}
if ($runtime -notmatch 'fetchVdp1ExtendedCommand\s*\(\s*\*cmd\s*\)') {
    throw 'Runtime no longer captures Azel extended-command depth'
}
if ($renderer -notmatch 'command\.hasDepth\s*&&\s*command\.depth\s*>=\s*0\.999f') {
    throw 'Renderer far-plane classification changed'
}

$backgroundCall = $renderer.IndexOf(
    'drawPublishedVdp1Ui(' + [Environment]::NewLine +
    '            Vdp1UiPass::Background')
$worldSubmit = $renderer.IndexOf('submit_vdp1_model(model, drawState)')
$foregroundCall = $renderer.IndexOf(
    'drawPublishedVdp1Ui(' + [Environment]::NewLine +
    '                Vdp1UiPass::Foreground')
if ($backgroundCall -lt 0 -or $worldSubmit -lt 0 -or $foregroundCall -lt 0 -or
    $backgroundCall -ge $worldSubmit -or $foregroundCall -le $worldSubmit) {
    throw 'VDP1 background/world/foreground composition order changed'
}

Write-Output 'PASS: authored far-plane VDP1 depth composes before world and foreground UI after world'
