$ErrorActionPreference = 'Stop'
# CPU arithmetic/source checks only, not hardware validation.
$root = Join-Path $PSScriptRoot '..'
$source = Get-Content -Raw (Join-Path $root 'src/platform/vita/neptune_renderer.cpp')
$source = $source.Replace("`r`n", "`n")
$solidBranch = [regex]::Match($source, '(?s)if \(command.fieldRadar && \(commandType == 4u.*?\n\s*continue;\n\s*\}').Value
foreach ($token in @('sceGxmSetVertexProgram(g_probeContext, g_textureVertexProgram)', 'sceGxmSetFragmentProgram(g_probeContext, g_textureFragmentProgram)', 'radarTextureUniforms, g_textureWvpParam, 0, 16, identity')) {
    if (!$solidBranch.Contains($token)) { throw "Radar solid-to-texture state restoration missing: $token" }
}
foreach ($token in @('command.fieldRadar && commandType == 0u', 'commandType == 4u || commandType == 6u', 'viewerRenderHeight() / 224', 'fieldRadarMap ? 0.0f', 'fieldRadarMap ? 1.0f')) {
    if (!$source.Contains($token)) { throw "Missing radar guard: $token" }
}
foreach ($case in @(@(480,272), @(960,544))) {
    $scale = [math]::Max(1, [math]::Floor($case[1] / 224))
    $frameLeft = $case[0] + (96 - 176) * $scale
    $orbLeft = $case[0] + (102 - 176) * $scale
    if ($frameLeft + 80 * $scale -ne $case[0] -or $orbLeft - $frameLeft -ne 6 * $scale) { throw 'Alignment wrong' }
    for ($pixel = 0; $pixel -lt 48 * $scale; ++$pixel) {
        $sample = [math]::Floor((($pixel + 0.5) / (48 * $scale)) * 48)
        if ($sample -ne [math]::Floor($pixel / $scale)) { throw 'Uneven checker repeats' }
    }
}
$point = [regex]::Match($source, '(?s)if \(fieldRadarMap\) \{\s*sceGxmTextureSetMinFilter.*?\n\s*\}').Value
if ([regex]::Matches($point, 'SCE_GXM_TEXTURE_FILTER_POINT').Count -ne 2) { throw 'Point filtering changed' }
Write-Output 'PASS: radar-only right anchor, shared integer frame/orb scale, uniform checker repeats, native fill/line routes (CPU/source checks only)'
