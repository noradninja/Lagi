param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib',[string]$ScratchRoot='..\point-selector-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
Push-Location $scratch
try {
 & cmake "-DREPO_ROOT=$repo" "-DLAGI_AZEL_SOURCE_ROOT=$AzelSourceRoot" -P (Join-Path $PSScriptRoot 'generate_native_menu_sources.cmake')
 if ($LASTEXITCODE -ne 0) { throw 'Actual selector source generation failed' }
 $source=Get-Content -Raw (Join-Path $scratch 'azel_upstream/3dEngine.cpp')
 $functions=@()
 foreach($name in @('resetProjectVectorMaster','pushProjectionStack_Master','dragonFieldTaskDrawSub3Sub1','initProjectionStack_Master','dragonFieldTaskDrawSub1Sub0')) {
  $match=[regex]::Match($source,"(?s)void $name\(\)\s*\{.*?\n\}")
  if (!$match.Success) {throw "Actual selector function missing: $name"}
  $functions+=$match.Value
 }
 Set-Content -LiteralPath (Join-Path $scratch 'point_selector_production.inc') -Value ($functions -join "`n")
 $exe=Join-Path $scratch 'point_selector_test.exe'
 & cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'point_light_selector_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
 if ($LASTEXITCODE -ne 0) {throw 'Selector host compilation failed'}
 & $exe
 if ($LASTEXITCODE -ne 0) {throw 'Selector lifecycle execution failed'}
 & E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 "-I$repo/include" "-I$AzelSourceRoot" (Join-Path $scratch 'azel_upstream/3dEngine.cpp')
 if ($LASTEXITCODE -ne 0) {throw 'Actual selector Vita syntax failed'}
 & E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 "-I$repo/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/field" (Join-Path $scratch 'azel_upstream/field_fieldDragon.cpp')
 if ($LASTEXITCODE -ne 0) {throw 'Actual field light restoration Vita syntax failed'}
} finally {Pop-Location}
