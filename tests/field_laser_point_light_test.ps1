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
    !$cmake.Contains('gouraud555_gradient_hue_at_peak(') -or
    !$cmake.Contains('m340_pLCS->m9C0') -or
    !$cmake.Contains('pThis->m6C[(pThis->m154 - 1) & 0xF]') -or
    !$cmake.Contains('lagi/lagi_render_bridge.h')) {
    throw 'Native LCS producer no longer uses beam-head position and drawn gradient hue'
}
if (!$cmake.Contains('pThis->mF0.m18_color[0][2]') -or
    !$cmake.Contains('colorIndex == 2') -or
    !$cmake.Contains('const s_RGB8 red = {s8(source.m0 ? 31 : 0), 0, 0};') -or
    !$cmake.Contains('[CollectionRedLight]') -or
    !$cmake.Contains('lagiSaturatedCollectionLightColor(colorIndex)') -or
    !$cmake.Contains('LAGI_COLLECTION_LIGHT_SATURATION') -or
    !$pool.Contains('saturate_rgb888_peak(')) {
    throw 'Homing red gradient, red collection class, or retained collection saturation is missing'
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
    !$renderer.Contains('accepted=lagi::evaluate_point_at_corner_prepared(') -or
    !$renderer.Contains('lagi::prepare_point_corner_matrix(transientWorld,nullptr,') -or
    !$renderer.Contains('evaluate_point_at_corner(world,light.additionalPointParams,') -or
    !$point.Contains('native_point_light_from_delta(') -or
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
if (!$renderer.Contains('static constexpr int kSceneModes[6] = {7, 8, 11, 9, 10, 12};') -or
    !$renderer.Contains('static int g_pendingViewMode = 7;')) {
    throw 'Debug cycle must place grayscale immediately before color while preserving default color'
}
if (!$renderer.Contains('if (mode == 7 || !buffer)') -or
    !$renderer.Contains('font.draw(buffer, viewerRenderWidth(), viewerRenderHeight(), pitch,') -or
    !$renderer.Contains('drawViewerModeOverlay(colorBuffer,gxmPitch,g_viewMode);')) {
    throw 'Diagnostic mode labels must use the native font at lower left, excluding Full'
}
if (!$cmake.Contains('LAGI_BATTLE_HOMING_POINT') -or
    !$cmake.Contains('LAGI_BATTLE_GUN_POINT') -or
    !$cmake.Contains('lagi::battle_gouraud555_hue_at_peak(rgb555, 248u)') -or
    !$bridge.Contains('[BattleLightEmit]') -or
    !$bridge.Contains('record_transient_battle_attack_light(') -or
    !$header.Contains('record_transient_battle_attack_light(')) {
    throw 'Native battle laser and gunshot producer hooks missing'
}
if (!$bridge.Contains('quarter_rgb888(packedColor)') -or
    !$bridge.Contains('constexpr std::uint32_t kBattleRange=0x5000u;') -or
    !$bridge.Contains('record_transient_laser_light(source,x,y,z,appliedColor,kBattleRange);') -or
    !$bridge.Contains('record_transient_laser_light(source,x,y,z,packedColor,0x14000u);') -or
    !$bridge.Contains('nativeRgb=%06X appliedRgb=%06X')) {
    throw 'Battle projectile light must retain quarter intensity and half-radius without changing field lights'
}
if (!$header.Contains('nativePrimaryPointPerCorner') -or
    !$renderer.Contains('!light.nativePrimaryPointPerCorner') -or
    !$bridge.Contains('state.nativePrimaryPointPerCorner=true')) {
    throw 'Native primary point source not redirected into shared evaluator'
}
if ($renderer.Contains('sampleSceneSkyAmbientAfterGpuFinish') -or
    $renderer.Contains('blend_sky_chroma_preserve_brightness') -or
    $renderer.Contains('g_skyAmbient') -or
    $renderer.Contains('skyFalloffMap') -or
    $renderer.Contains('[BattleSkyAmbient]') -or
    !$renderer.Contains('falloffMap[depthIndex][0],') -or
    !$renderer.Contains('falloffMap[depthIndex][1],') -or
    !$renderer.Contains('falloffMap[depthIndex][2]')) {
    throw 'Sky sampling or chroma remains in the native authored-ambient path'
}
Write-Output 'PASS: field/battle light producers, combat half-radius, authored ambient, shared four-corner evaluator and cached lighting'
