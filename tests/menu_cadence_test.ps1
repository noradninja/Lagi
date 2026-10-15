param([string]$ScratchRoot='..\menu-cadence-test',[string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
Push-Location $scratch
try {
 & cmake "-DREPO_ROOT=$repo" "-DLAGI_AZEL_SOURCE_ROOT=$AzelSourceRoot" -P (Join-Path $PSScriptRoot 'generate_native_menu_sources.cmake')
 if($LASTEXITCODE -ne 0){throw 'Native source generation failed'}
} finally {Pop-Location}
$native=Join-Path $scratch 'azel_upstream/mainMenuDebugTasks.cpp'
$source=Get-Content -Raw $native
$begin=$source.IndexOf('void s_mainMenuWorkArea::Delete(s_mainMenuWorkArea* pWorkArea)')
$end=$source.IndexOf('p_workArea createMainMenuTask(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production native cadence restore missing'}
Set-Content -LiteralPath (Join-Path $scratch 'menu_cadence_delete_production.inc') -Value $source.Substring($begin,$end-$begin)
$source=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$begin=$source.IndexOf('static void waitForPresentSlot(')
$end=$source.IndexOf('static void recordFlightPresentation(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production display clock missing'}
Set-Content -LiteralPath (Join-Path $scratch 'menu_cadence_clock_production.inc') -Value $source.Substring($begin,$end-$begin)
$begin=$source.IndexOf('void presentation_set_native_frame_vblanks(')
$end=$source.IndexOf('} // namespace lagi::platform::renderer',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production cadence staging missing'}
Set-Content -LiteralPath (Join-Path $scratch 'menu_cadence_setter_production.inc') -Value $source.Substring($begin,$end-$begin)
if(!$source.Contains('g_nativeFrameVblanks = g_pendingNativeFrameVblanks;') -or
 !$source.Contains('waitForPresentSlot(nativeSceneMode ? g_nativeFrameVblanks : 2u);')){throw 'Native cadence publication/consumer missing'}
$exe=Join-Path $scratch 'menu_cadence_test.exe'
& cl /nologo /std:c++20 /EHsc (Join-Path $PSScriptRoot 'menu_cadence_test.cpp') "/I$scratch" "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Cadence host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Cadence host execution failed'}
& E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 "-I$repo/include" "-I$AzelSourceRoot" $native
if($LASTEXITCODE -ne 0){throw 'Generated menu Vita syntax failed'}
