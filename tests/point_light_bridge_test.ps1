param([string]$ScratchRoot='..\point-bridge-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
foreach($entry in @(
 @('src/integration/lagi_render_bridge.cpp','static void capture_native_point_light(','void capture_current_light(','point_capture_production.inc'),
 @('src/platform/vita/neptune_renderer.cpp','static void transformLiveLightVectorToModelSpace(','static void appendLiveTownModel(','point_consumer_production.inc'))) {
 $source=Get-Content -Raw (Join-Path $repo $entry[0])
 $begin=$source.IndexOf($entry[1]); $end=$source.IndexOf($entry[2],$begin)
 if ($begin -lt 0 -or $end -le $begin) {throw 'Production point bridge boundary missing'}
 $body=$source.Substring($begin,$end-$begin).Replace('const azel_bridge::SubmissionState&','const SubmissionState&')
 Set-Content -LiteralPath (Join-Path $scratch $entry[3]) -Value $body
}
$exe=Join-Path $scratch 'point_bridge_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'point_light_bridge_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if ($LASTEXITCODE -ne 0) {throw 'Point bridge host compilation failed'}
& $exe
if ($LASTEXITCODE -ne 0) {throw 'Point bridge execution failed'}
