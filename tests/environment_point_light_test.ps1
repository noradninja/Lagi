param([string]$ScratchRoot='../environment-point-test',[string]$AzelRoot='E:/dev/Lagi/extern/Azel/AzelLib')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_render_bridge.cpp"
foreach($entry in @(
 @('struct FieldEnvironmentPointLight','// Bounded FLD_A3 diagnostic:','environment_state.inc'),
 @('void begin_frame(bool forceDynamicSubmissions)','std::uint32_t submission_count()','environment_begin.inc'),
 @('static void capture_native_point_light(','void capture_field_environment_point_light()','actor_point_capture.inc'),
 @('void capture_current_light(','void begin_view_relative_submission_scope()','actor_current_capture.inc'),
 @('void capture_field_environment_point_light()','void capture_current_light(','environment_capture.inc'))){
 $start=$source.IndexOf($entry[0]);$end=$source.IndexOf($entry[1],$start)
 if($start -lt 0 -or $end -lt 0){throw "Production environment boundary missing: $($entry[0])"}
 Set-Content "$scratch/$($entry[2])" $source.Substring($start,$end-$start)
}
if(!$source.Contains('capture_native_point_light(g_lastState);'+"`r`n"+'        apply_field_environment_point_light(g_lastState);') -and
 !$source.Contains('capture_native_point_light(g_lastState);'+"`n"+'        apply_field_environment_point_light(g_lastState);')){throw 'Explicit environment submission hook missing'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$renderer.Contains('lagi::accumulate_additional_model_light(light.additionalLight,lighting.normal,accum);') -or
 !$renderer.Contains('polygonLight.additionalLight=state.additionalLight;')){throw 'Shared additive corner shading or dynamic payload hook missing'}
foreach($entry in @(
 @('struct LivePolygonLightState {','static std::vector<LivePolygonLightState>','environment_polygon_type.inc'),
 @('static void transformLiveLightVectorToModelSpace(','static void appendLiveTownModel(','environment_consumer.inc'))){
 $start=$renderer.IndexOf($entry[0]);$end=$renderer.IndexOf($entry[1],$start)
 if($start -lt 0 -or $end -lt 0){throw 'Renderer light extraction boundary missing'}
 Set-Content "$scratch/$($entry[2])" $renderer.Substring($start,$end-$start).Replace('const azel_bridge::SubmissionState&','const SubmissionState&')
}
$refresh=$renderer.IndexOf('static void refreshLiveTownStaticLighting()')
$start=$renderer.IndexOf('        LivePolygonLightState light{};', $refresh)
$end=$renderer.IndexOf('        if (fieldFalloffCompareBudget', $start)
if($refresh -lt 0 -or $start -lt 0 -or $end -lt 0){throw 'Cached mesh lighting refresh boundary missing'}
Set-Content "$scratch/environment_cached_payload.inc" $renderer.Substring($start,$end-$start)
Push-Location $scratch
try {
 & cmake "-DREPO_ROOT=$repo" "-DLAGI_AZEL_SOURCE_ROOT=$AzelRoot" -P "$PSScriptRoot/generate_native_menu_sources.cmake"
 if($LASTEXITCODE -ne 0){throw 'Production generation failed'}
} finally {Pop-Location}
$dragon=Get-Content -Raw "$scratch/azel_upstream/field_fieldDragon.cpp"
$grid=Get-Content -Raw "$scratch/azel_upstream/field_field_a3_o_fld_a3.cpp"
if([regex]::Matches($dragon,'capture_field_environment_point_light\(\)').Count -ne 1 -or
 !$dragon.Contains('LAGI_FIELD_ACTOR_ADDITIVE_BASE') -or
 !$dragon.Contains('pTypedWorkArea->mC8_normalLightColor.toU32());') -or
 !$dragon.Contains('if (pTypedWorkArea->m_EB_useSpecialColor)') -or
 !$dragon.Contains('LAGI_NATIVE_FIELD_LIGHT_RESTORE') -or
 !$grid.Contains('lagiState.fieldEnvironment = true; // LAGI_FIELD_ENVIRONMENT_POINT_TARGET')){throw 'Generated source/target/cleanup hooks missing'}
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/environment_point_light_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Environment point host compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Environment point host test failed'}
