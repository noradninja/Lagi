$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$cmake=Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$bridge=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_render_bridge.cpp')
$header=Get-Content -Raw (Join-Path $repo 'include/lagi/lagi_render_bridge.h')
$renderer=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$pool=Get-Content -Raw (Join-Path $repo 'include/lagi/lagi_transient_laser_lights.h')
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
    !$renderer.Contains('native_point_light(modelAtCorner,params)') -or
    !$renderer.Contains('g_liveTownCpuMesh.vertices[p*6u+vertexIndex]') -or
    !$renderer.Contains('accumulate_additional_model_light(vertexLight,lighting.normal,accum);')) {
    throw 'Missing four-light snapshot or additive Gouraud integration'
}
if (!$renderer.Contains('[FieldLaserCoverage]') -or
    !$renderer.Contains('changedRGB555=%u') -or
    !$renderer.Contains('rejectedPolys=%u') -or
    !$renderer.Contains('light.transientLaserLightCount')) {
    throw 'Missing bounded diagnostic for field laser polygon coverage'
}
Write-Output 'PASS: field LCS laser producer, shared four-light cap, frame reset, native falloff and additive Gouraud source assertions'
