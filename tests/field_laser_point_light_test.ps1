$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$cmake=Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$bridge=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_render_bridge.cpp')
$header=Get-Content -Raw (Join-Path $repo 'include/lagi/lagi_render_bridge.h')
$renderer=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$pool=Get-Content -Raw (Join-Path $repo 'include/lagi/lagi_transient_laser_lights.h')
$point=Get-Content -Raw (Join-Path $repo 'include/lagi/lagi_corner_point_light.h')
if (!$pool.Contains('kMaxTransientLaserLights = 4') -or
    !$pool.Contains('if (count == kMaxTransientLaserLights) return false;')) {
    throw 'Native transient pool no longer enforces the hard cap of four'
}
if (!$cmake.Contains('LAGI_FIELD_LASER_LIGHT') -or
    !$cmake.Contains('const s_RGB8& nativeColor = s_LCSLaser_DeleteTrailSub0Data0[pThis->m27]') -or
    !$cmake.Contains('pThis->m6C[(pThis->m154 - 1) & 0xF]') -or
    !$cmake.Contains('lagi/lagi_render_bridge.h')) {
    throw 'Native LCS producer no longer uses beam-head position and authored LCS color'
}
if (!$bridge.Contains('g_completedFieldLaserLights=g_fieldLaserLights;') -or
    !$bridge.Contains('g_completedFieldLaserLights.count') -or
    !$bridge.Contains('g_fieldLaserLights.clear();') -or
    !$bridge.Contains('apply_transient_field_laser_lights(g_lastState);') -or
    !$bridge.Contains('native_point_light(nativeMatrix,params)') -or
    !$bridge.Contains('state.transientLaserSources[slot]=source;') -or
    !$bridge.Contains('state.transientLaserLightCount==lagi::kMaxTransientLaserLights')) {
    throw 'Transient field lights must be frame-owned and native range bounded'
}
if (!$header.Contains('std::array<AdditionalModelLight, lagi::kMaxTransientLaserLights>') -or
    !$renderer.Contains('polygonLight.transientLaserLights=state.transientLaserLights;') -or
    !$renderer.Contains('transient<light.transientLaserLightCount') -or
    !$renderer.Contains('evaluate_point_at_corner(world,params,') -or
    !$renderer.Contains('evaluate_point_at_corner(world,light.additionalPointParams,') -or
    !$point.Contains('native_point_light(matrix,params)') -or
    !$renderer.Contains('accumulate_additional_model_light(vertexLight,lighting.normal,accum);')) {
    throw 'Missing four-light snapshot or additive Gouraud integration'
}
if (!$renderer.Contains('[FieldLaserCoverage]') -or
    !$renderer.Contains('changedRGB555=%u') -or
    !$renderer.Contains('rejectedPolys=%u') -or
    !$renderer.Contains('light.transientLaserLightCount')) {
    throw 'Missing bounded diagnostic for field laser polygon coverage'
}
# Static field meshes are cached, so the per-frame refresh MUST propagate
# exactly the same transient fields as the dynamic model append path.
$staticStart=$renderer.IndexOf('static void refreshLiveTownStaticLighting()')
$staticEnd=$renderer.IndexOf('// Shared with submission:', $staticStart)
if ($staticStart -lt 0 -or $staticEnd -le $staticStart) {
    throw 'Static field light refresh function not found'
}
$staticRefresh=$renderer.Substring($staticStart,$staticEnd-$staticStart)
foreach ($field in @('transientLaserLights', 'transientLaserSources',
                    'transientLaserLightCount', 'modelBasis')) {
    if (!$staticRefresh.Contains("light.$field")) {
        throw "Static light refresh fails to propagate $field"
    }
}
if (!$renderer.Contains('static constexpr int kSceneModes[5] = {10, 7, 8, 9, 11};') -or
    !$renderer.Contains('static int g_pendingViewMode = 7;')) {
    throw 'Debug cycle must place grayscale immediately before color while preserving default color'
}
if (!$cmake.Contains('LAGI_BATTLE_HOMING_POINT') -or
    !$cmake.Contains('LAGI_BATTLE_GUN_POINT') -or
    !$bridge.Contains('record_transient_battle_attack_light(') -or
    !$header.Contains('record_transient_battle_attack_light(')) {
    throw 'Native battle laser and gunshot producer hooks missing'
}
if (!$header.Contains('nativePrimaryPointPerCorner') -or
    !$renderer.Contains('!light.nativePrimaryPointPerCorner') -or
    !$bridge.Contains('state.nativePrimaryPointPerCorner=true')) {
    throw 'Native primary point source not redirected into shared evaluator'
}
if (!$renderer.Contains('sampleSceneSkyAmbientAfterGpuFinish();') -or
    !$renderer.Contains('g_skyAmbientValid && accum[0]==accum[1]') -or
    !$renderer.Contains('g_sceneRbg0ColorBuffer[y*kSceneRbg0Pitch+x]')) {
    throw 'Grayscale-only ambient sky sampling or gating missing'
}
Write-Output 'PASS: field laser cap, shared native four-corner evaluator, cached static lighting and debug order'
