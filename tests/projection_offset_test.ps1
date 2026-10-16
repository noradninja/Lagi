param([string]$ScratchRoot='..\projection-offset-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$bridge=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_render_bridge.cpp')
$scopeBegin=$bridge.IndexOf('void begin_view_relative_submission_scope(')
$scopeEnd=$bridge.IndexOf('void capture_native_scene_view_matrix()',$scopeBegin)
if($scopeBegin -lt 0 -or $scopeEnd -le $scopeBegin){throw 'Native preview scope missing'}
Set-Content (Join-Path $scratch 'projection_scope_production.inc') $bridge.Substring($scopeBegin,$scopeEnd-$scopeBegin)
Push-Location $scratch
try {
 & cmake "-DREPO_ROOT=$repo" -DLAGI_AZEL_SOURCE_ROOT=E:/dev/Lagi/extern/Azel/AzelLib -P "$PSScriptRoot/generate_native_menu_sources.cmake"
 if($LASTEXITCODE -ne 0){throw 'Native preview generation failed'}
} finally {Pop-Location}
$preview=Get-Content -Raw (Join-Path $scratch 'azel_upstream/menu_dragonMorph.cpp')
if(!$preview.Contains('begin_view_relative_submission_scope(true);') -or !$preview.Contains('LAGI_MENU_PREVIEW_NATIVE_VIEW')){throw 'Preview native root capture not hooked'}
$capture=$bridge.IndexOf('lagi::scene_bridge::capture_local_coordinates(')
$publish=$bridge.IndexOf('std::int32_t adaptedIndex = -1;', $capture)
if($capture -lt 0 -or $publish -le $capture -or
 !$bridge.Substring($capture,$publish-$capture).Contains('g_lastState.dynamic = true;')) {
 throw 'Submission capture or shifted-model cache bypass missing'
}
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_scene_bridge.cpp')
$begin=$source.IndexOf('void capture_local_coordinates(')
$end=$source.IndexOf('namespace {',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native coordinate capture missing'}
Set-Content -LiteralPath (Join-Path $scratch 'projection_capture_production.inc') -Value $source.Substring($begin,$end-$begin)
$renderer=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
if(!$renderer.Contains('lagi::nativeProjectionOffset(state.localCoordinatesX,') -or
 !$renderer.Contains('v.x += projectionOffset[0]*viewZ;') -or
 !$renderer.Contains('v.y += projectionOffset[1]*viewZ;') -or
 !$renderer.Contains('v.z += projectionOffset[2]*viewZ;')) {
 throw 'Native per-model projection consumer missing'
}
$exe=Join-Path $scratch 'projection_offset_test.exe'
& cl /nologo /std:c++20 /EHsc /DLAGI_TEST_CAPTURE "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'projection_offset_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Projection test compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Projection test execution failed'}
