param(
 [string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib',
 [string]$VitaCompiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe',
 [string]$ScratchRoot=(Join-Path $env:TEMP 'lagi-native-menu-pipeline')
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$null=New-Item -ItemType Directory -Force -Path $ScratchRoot
$ScratchRoot=(Resolve-Path $ScratchRoot).Path
Push-Location $ScratchRoot
try {
 & cmake "-DREPO_ROOT=$root" "-DLAGI_AZEL_SOURCE_ROOT=$AzelSourceRoot" -P (Join-Path $PSScriptRoot 'generate_native_menu_sources.cmake')
 if ($LASTEXITCODE -ne 0) {throw 'Actual native source generation failed'}
 $generated=Join-Path $ScratchRoot 'azel_upstream'
 foreach($name in @('field_field_a3_o_fld_a3.cpp','field_fieldRadar.cpp','LCS.cpp','kernel_dialogTask.cpp','VDP2.cpp')) {
  & $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/field" "-I$AzelSourceRoot/field/field_a3" "-I$AzelSourceRoot/kernel" (Join-Path $generated $name)
  if ($LASTEXITCODE -ne 0) {throw "Generated Vita syntax failed: $name"}
 }
 foreach($source in @('lagi_azel_field_navigator.cpp','lagi_azel_field_radar.cpp','lagi_runtime.cpp','lagi_heap_vita.cpp','lagi_vdp1_animated_quad_vita.cpp')) {
  & $VitaCompiler -std=c++20 -fsyntax-only -DSHIPPING_BUILD=1 -DLAGI_PLATFORM_VITA=1 -DLAGI_BATTLE_BUILD_NATIVE=1 -DLAGI_BATTLE_BUILD_EXECUTE_A3=1 -DLAGI_BATTLE_BUILD_LINK_AUDIT=0 "-I$root/include" "-I$AzelSourceRoot" (Join-Path $root "src/integration/$source")
  if ($LASTEXITCODE -ne 0) {throw "Native service Vita syntax failed: $source"}
 }
 $dispatcher=Get-Content -Raw (Join-Path $generated 'field_field_a3_o_fld_a3.cpp')
 foreach($address in @('0x06073172','0x06073120','0x06072f3c','0x060730bc')) {
  if ([regex]::Matches($dispatcher,[regex]::Escape("case ${address}:")).Count -ne 1) {throw "Navigator dispatch missing/duplicated: $address"}
 }
 $dialog=Get-Content -Raw (Join-Path $generated 'kernel_dialogTask.cpp')
 if (!$dialog.Contains('*pThis->mC_result = -1; // LAGI_NATIVE_DIALOG_CANCEL') -or !$dialog.Contains('lagiReleaseFieldNavigatorDialog(pThis);')) {throw 'Native cancel/lifetime hooks missing'}
 $lcs=Get-Content -Raw (Join-Path $generated 'LCS.cpp')
 $dragon=Get-Content -Raw (Join-Path $generated 'field_fieldDragon.cpp')
 if (!$dragon.Contains('transformAndAddVecByCurrentMatrix(&varC, &lightLocation); // LAGI_NATIVE_POINT_POSITION') -or
     !$dragon.Contains('transformVecByCurrentMatrix(varC, lightLocation);')) {throw 'Point position translation or directional vector transform missing'}
 if ($lcs.Contains('pDragon->m_EC = rotation;') -or !$lcs.Contains('LAGI_SPARKLE_FLAGS_PRESERVED')) {throw 'Sparkle orbit corrupts dragon lighting flags'}
 if (!$dragon.Contains('lightLocation = pTypedWorkArea->mD8_sparklePosition; // LAGI_COLLECTION_ANIMATED_LIGHT') -or
     !$dragon.Contains('mE4_sparkleScale.m_value == 0x10000')) {throw 'Animated collection light does not reach native A11D draw'}
 foreach($token in @('LAGI_TRAIL_LIGHT_START','LAGI_TRAIL_LIGHT_FOLLOW','lagiStartCollectionTrailLight(s32 colorIndex)',
                    's_LCSLaser_DeleteTrailSub0Data0[colorIndex]','pThis->m154 < 0x11',
                    'pThis->m24_receivedItemId >= 0')) {
  if (!$dispatcher.Contains($token)) {throw "Collection light missing native trail ownership: $token"}
 }
 if (!$lcs.Contains('LAGI_TRAIL_LIGHT_CLEANUP') -or
     !$lcs.Contains('mD8_sparklePosition') -or
     !$lcs.Contains('mE4_sparkleScale = 0;')) {throw 'Collection light sparkle handoff/cleanup missing'}
 if (!$lcs.Contains('lagiDrawFieldRadarObjectMarkers(r4);')) {throw 'Object-marker traversal hook missing'}
 if (!$lcs.Contains('pThis->m2C_localOffset[2] = pThis->m34_randomizedLifetime;') -or
     !$lcs.Contains('displayRaySegment_2Width(radial, widths, base + 0x6B0, 0x031E, base + 0x1780, &colors, 8);')) {throw 'Native radial collection burst hooks missing'}
 Write-Output 'PASS: actual production-generated navigator/radar/dialog/glyph adapters and service Vita syntax; not device execution or full linking'
} finally {Pop-Location}
